#include "Utility.h"
#include <comdef.h>
#include <fstream>
#include <iostream>

#include <directxtk12/DDSTextureLoader.h>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
    : ErrorCode(hr), FunctionName(functionName), Filename(filename), LineNumber(lineNumber)
{
}

std::wstring DxException::ToString() const
{
    // HRESULT 코드를 사람이 읽을 수 있는 시스템 오류 메시지로 변환한다.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

namespace GraphicsUtils
{

using Microsoft::WRL::ComPtr;

Utility::Utility() : m_device(nullptr)
{
}

Utility::Utility(ID3D12Device5* pDevice) : m_device(pDevice)
{
}

void Utility::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    // TODO: IDXGIOutput::GetDisplayModeList 를 호출해 지원 해상도/주사율을 출력한다.
}

void Utility::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                   ComPtr<ID3D12DescriptorHeap>& heap, UINT nodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flag)
{
    // 기본 생성자로 만들어진 Utility 는 디바이스가 없으므로 안전하게 무시한다.
    if (m_device == nullptr)
        return;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = flag;
    heapDesc.NodeMask = nodeMask;
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;

    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));
}

void Utility::CreateConstantBuffer(UINT bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, void** pConstant)
{
    // CPU 가 매 프레임 갱신할 수 있도록 UPLOAD 힙에 상수 버퍼를 배치한다.
    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                    D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                    IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));

    // CPU 는 읽지 않으므로 read range 를 비워두어 드라이버 최적화를 유도한다.
    // 매핑은 해제하지 않고 유지(persistent map)해 매 프레임 memcpy 만으로 갱신한다.
    CD3DX12_RANGE range(0, 0);
    ThrowIfFailed(buffer->Map(0, &range, pConstant));
}

D3D12_SHADER_RESOURCE_VIEW_DESC Utility::CreateSRVDesc(ID3D12Resource* resource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = resource->GetDesc().Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

    return srvDesc;
}

void Utility::CreateTextureBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, UINT width, UINT height,
                                  DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state,
                                  UINT mipLevels, std::wstring name)
{
    // 2D 텍스처 디스크립션 구성. 멀티샘플은 사용하지 않는다.
    D3D12_RESOURCE_DESC rDesc = {};
    rDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rDesc.DepthOrArraySize = 1;
    rDesc.Width = width;
    rDesc.Height = height;
    rDesc.Format = format;
    rDesc.SampleDesc = {1, 0};
    rDesc.Flags = flags;
    rDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rDesc.MipLevels = mipLevels;

    // 깊이 버퍼 생성 시 typeless 포맷을 받았다면 실제 클리어용 포맷으로 치환한다.
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = format;
    if (format == DXGI_FORMAT_R32_TYPELESS)
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;

    // 깊이/스텐실 포맷이면 Depth=1.0, Stencil=0 으로, 그 외 컬러 포맷이면 clearValue를 설정하지 않는다
    if (clearValue.Format == DXGI_FORMAT_D24_UNORM_S8_UINT || clearValue.Format == DXGI_FORMAT_D32_FLOAT)
    {
        clearValue.DepthStencil.Depth = 1.f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                        D3D12_HEAP_FLAG_NONE, &rDesc, state, &clearValue,
                                                        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
    }
    else
    {
        ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                        D3D12_HEAP_FLAG_NONE, &rDesc, state, nullptr,
                                                        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
    }
    // PIX 등 디버깅 도구에서 식별 가능하도록 리소스에 이름을 부여한다.
    buffer->SetName(name.c_str());
}

void Utility::CreateResourceView(ID3D12Resource* resource, DXGI_FORMAT format, bool bUseMsaa,
                                 D3D12_CPU_DESCRIPTOR_HANDLE& handle, const DescriptorType& type,
                                 const ViewDimensionType& viewType, UINT miplevel)
{
    // 요청된 뷰 종류에 따라 분기. 동일한 리소스라도 사용 목적에 맞는 뷰를 별도로 생성해야 한다.
    if (type == DescriptorType::RTV)
    {
        if (viewType == ViewDimensionType::TEXTURECUBE)
        {
            std::cout << "RTV는 TEXTURECUBE를 사용할 수 없습니다" << std::endl;
            return;
        }
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
        ZeroMemory(&rtvDesc, sizeof(rtvDesc));
        if (bUseMsaa)
        {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        }
        rtvDesc.Format = format;
        rtvDesc.Texture2D.MipSlice = 0;

        m_device->CreateRenderTargetView(resource, &rtvDesc, handle);
    }
    else if (type == DescriptorType::UAV)
    {
        // UAV 는 멀티샘플 텍스처에 바인딩할 수 없다. 잘못된 호출을 방지한다.
        if (bUseMsaa)
        {
            std::cout << "UAV는 MSAA를 사용할 수 없습니다" << std::endl;
            return;
        }
        if (viewType == ViewDimensionType::TEXTURECUBE)
        {
            std::cout << "UAV는 TEXTURECUBE를 사용할 수 없습니다" << std::endl;
            return;
        }
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));

        uavDesc.Format = format;
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Format = format;
        uavDesc.Texture2D.MipSlice = 0;
        m_device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, handle);
    }
    else if (type == DescriptorType::SRV)
    {

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (bUseMsaa)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            srvDesc.ViewDimension = viewType == ViewDimensionType::TEXTURE2D ? D3D12_SRV_DIMENSION_TEXTURE2D
                                                                             : D3D12_SRV_DIMENSION_TEXTURECUBE;
        }
        srvDesc.Format = format;
        srvDesc.Texture2D.MipLevels = miplevel;
        m_device->CreateShaderResourceView(resource, &srvDesc, handle);
    }
    else if (type == DescriptorType::DSV)
    {
        if (viewType == ViewDimensionType::TEXTURECUBE)
        {
            std::cout << "DSV는 TEXTURECUBE를 사용할 수 없습니다" << std::endl;
            return;
        }
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.Format = format;

        m_device->CreateDepthStencilView(resource, &dsvDesc, handle);
    }
}

void Utility::CreateStructuredResourceView(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, DXGI_FORMAT format,
                                           D3D12_CPU_DESCRIPTOR_HANDLE& handle, const DescriptorType& type, UINT count,
                                           UINT dataSize)
{
    if (type == DescriptorType::UAV)
    {

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));

        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = count;
        uavDesc.Buffer.StructureByteStride = dataSize;

        m_device->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, handle);
    }
    else if (type == DescriptorType::SRV)
    {

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = count;
        srvDesc.Buffer.StructureByteStride = dataSize;

        m_device->CreateShaderResourceView(buffer.Get(), &srvDesc, handle);
    }
}

void Utility::CreateTextureFromDDS(const uint8_t* ddsBytes, size_t ddsSize, ComPtr<ID3D12Resource>& gpu,
                                   ComPtr<ID3D12Resource>& upload, DirectX::DDS_LOADER_FLAGS loadFlags,
                                   ID3D12GraphicsCommandList* commandList)
{
    // (1) DDS 헤더 파싱 → DEFAULT 힙 텍스처 생성. 데스크탑(non-Xbox) 빌드에서 초기 상태는 COMMON.
    //     subresources[].pData 는 ddsBytes 내부를 가리키므로 호출자가 ExecuteCommandLists 완료까지 alive 유지해야 한다.
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    ThrowIfFailed(DirectX::LoadDDSTextureFromMemoryEx(m_device, ddsBytes, ddsSize, 0, D3D12_RESOURCE_FLAG_NONE,
                                                      loadFlags, gpu.ReleaseAndGetAddressOf(), subresources));

    // (2) mip / face 를 모두 담을 수 있는 UPLOAD 힙 크기 계산 (row pitch 정렬 포함).
    const UINT64 uploadSize = GetRequiredIntermediateSize(gpu.Get(), 0, static_cast<UINT>(subresources.size()));

    // (3) UPLOAD 힙 생성. CreateBuffer 와 동일 패턴.
    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                    D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
                                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                    IID_PPV_ARGS(upload.ReleaseAndGetAddressOf())));

    // (4) COMMON → COPY_DEST. CreateBuffer (3) 과 같은 전이.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                          D3D12_RESOURCE_STATE_COPY_DEST));

    // (5) 모든 mip / face 를 한 번에 복사 기록. CreateBuffer 의 단일 subresource 와 호출 형태만 다르다.
    UpdateSubresources(commandList, gpu.Get(), upload.Get(), 0, 0, static_cast<UINT>(subresources.size()),
                       subresources.data());

    // (6) COPY_DEST → PIXEL_SHADER_RESOURCE. PS sampling 외 용도면 호출자가 별도 barrier 처리.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_COMMON));
}

ByteArray ReadFileHelper(const wstring& fileName)
{
    // 실패 시 호출자에게 빈 ByteArray 를 반환하기 위한 센티넬 값.
    ByteArray NullFile = make_shared<vector<byte>>(vector<byte>());

    // 파일 크기를 한 번에 읽기 위해 사전에 stat 으로 크기를 확인한다.
    struct _stat64 fileStat;
    int fileExists = _wstat64(fileName.c_str(), &fileStat);
    if (fileExists == -1)
        return NullFile;

    ifstream file(fileName, ios::in | ios::binary);
    if (!file)
        return NullFile;

    // 파일 크기에 맞는 버퍼를 미리 할당해 한 번의 read 호출로 모두 읽는다.
    ByteArray byteArray = make_shared<vector<byte>>(fileStat.st_size);
    file.read((char*)byteArray->data(), byteArray->size());
    file.close();

    return byteArray;
}
} // namespace GraphicsUtils
