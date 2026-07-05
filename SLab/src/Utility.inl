#pragma once

#include "Utility.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace GraphicsUtils
{
template <typename DataType>
inline void Utility::CreateBuffer(DataType* data, UINT64 bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& upload, D3D12_RESOURCE_FLAGS flag,
                                  ID3D12GraphicsCommandList* commandList)
{
    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                    D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                    IID_PPV_ARGS(upload.ReleaseAndGetAddressOf())));

    // (2) 실제 셰이더가 사용할 GPU 전용 버퍼. DEFAULT 힙은 GPU 측 접근이 가장 빠르다.
    ThrowIfFailed(
        m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                          &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flag), D3D12_RESOURCE_STATE_COMMON,
                                          nullptr, IID_PPV_ARGS(gpu.ReleaseAndGetAddressOf())));

    // 업로드할 원본 데이터에 대한 디스크립션 구성. 버퍼이므로 RowPitch == SlicePitch == 전체 크기.
    D3D12_SUBRESOURCE_DATA subData;
    subData.pData = static_cast<void*>(data);
    subData.RowPitch = bufferSize;
    subData.SlicePitch = subData.RowPitch;

    // (3) UpdateSubresources 가 복사 명령을 기록하기 전에 COPY_DEST 상태로 전이.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                          D3D12_RESOURCE_STATE_COPY_DEST));

    // (4) 헬퍼 함수가 필요한 풋프린트 계산과 CopyBufferRegion 명령 기록을 모두 처리한다.
    UpdateSubresources(commandList, gpu.Get(), upload.Get(), 0, 0, 1, &subData);

    // (5) 이후 어떤 용도로든 사용할 수 있도록 COMMON 상태로 되돌린다.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_COMMON));
}
template <typename DataType>
inline void Utility::CreateUploadBuffer(DataType* data, UINT64 bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                                        D3D12_RESOURCE_FLAGS flag, ID3D12GraphicsCommandList* commandList)
{
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flag), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(gpu.ReleaseAndGetAddressOf())));
}

template <typename DataType>
inline void Utility::CreateBuffer(const std::vector<DataType>& data, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& upload, D3D12_RESOURCE_FLAGS flag,
                                  ID3D12GraphicsCommandList* commandList)
{
    // (1) CPU → GPU 전송용 임시 버퍼. UPLOAD 힙은 CPU 매핑이 가능하다.
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(DataType) * data.size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(upload.ReleaseAndGetAddressOf())));

    // (2) 실제 셰이더가 사용할 GPU 전용 버퍼. DEFAULT 힙은 GPU 측 접근이 가장 빠르다.
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(DataType) * data.size(), flag), D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(gpu.ReleaseAndGetAddressOf())));

    // 업로드할 원본 데이터에 대한 디스크립션 구성. 버퍼이므로 RowPitch == SlicePitch == 전체 크기.
    D3D12_SUBRESOURCE_DATA subData;
    subData.pData = data.data();
    subData.RowPitch = sizeof(DataType) * data.size();
    subData.SlicePitch = subData.RowPitch;

    // (3) UpdateSubresources 가 복사 명령을 기록하기 전에 COPY_DEST 상태로 전이.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                          D3D12_RESOURCE_STATE_COPY_DEST));

    // (4) 헬퍼 함수가 필요한 풋프린트 계산과 CopyBufferRegion 명령 기록을 모두 처리한다.
    UpdateSubresources(commandList, gpu.Get(), upload.Get(), 0, 0, 1, &subData);

    // (5) 이후 어떤 용도로든 사용할 수 있도록 COMMON 상태로 되돌린다.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_COMMON));
}
inline void Utility::CreateBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, D3D12_HEAP_TYPE heapType, UINT64 size,
                                  D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, std::wstring name)
{
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size, flags), state, nullptr,
                                                    IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
    buffer->SetName(name.c_str());
}

inline std::string Utility::MakeTimestamp()
{
    using namespace std::chrono;

    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "_%y%m%d_%H%M%S");
    return oss.str();
}

} // namespace GraphicsUtils
