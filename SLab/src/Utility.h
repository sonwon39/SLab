#pragma once

#include <string>
#include <vector>
#include <memory>

#include "wrl.h"
#include "d3d12.h"
#include "directx/d3dx12.h"
#include "dxgi1_6.h"
#include <directxtk12/SimpleMath.h>
#include <directxtk12/DDSTextureLoader.h>

#include "DataType.h"

// #include "StaticMesh.h"
class StaticMesh;

/** D3D12 GPU 가상 주소가 비어있음을 나타내는 상수. (NULL 주소) */
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)0)
/** D3D12 GPU 가상 주소가 아직 결정되지 않았음을 나타내는 상수. */
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN ((D3D12_GPU_VIRTUAL_ADDRESS) - 1)

template <typename V, typename I> class Mesh;

/**
 * ANSI(MBCS) 문자열을 와이드 문자열(UTF-16)로 변환한다.
 * D3D12 API 및 디버그 출력에서 와이드 문자열이 요구되는 경우 사용한다.
 *
 * @param str 변환할 ANSI 문자열.
 * @return    변환된 std::wstring 객체.
 *
 * @note 내부 버퍼 크기는 512자로 고정되어 있어 그 이상의 입력은 잘려나갈 수 있다.
 */
inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

/** 임의의 객체 크기를 4바이트(UINT32) 단위로 올림 계산한다. (루트 상수 등록 시 사용) */
#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

#ifndef ThrowIfFailed
/**
 * HRESULT를 반환하는 D3D12 API 호출을 감싸는 매크로.
 * 호출이 실패하면 호출 위치(파일/라인)와 함수명을 담은 DxException을 throw 한다.
 */
#define ThrowIfFailed(x)                                                                                               \
    {                                                                                                                  \
        HRESULT hr__ = (x);                                                                                            \
        std::wstring wfn = AnsiToWString(__FILE__);                                                                    \
        if (FAILED(hr__))                                                                                              \
        {                                                                                                              \
            throw DxException(hr__, L#x, wfn, __LINE__);                                                               \
        }                                                                                                              \
    }
#endif

#if defined(DEBUG) || (_DEBUG)

/**
 * 디버그 빌드에서만 동작하는 단언(assertion) 매크로.
 * 조건이 거짓이면 디버거 브레이크를 발생시켜 즉시 실행을 중단한다.
 */
#define ASSERT(isFalse, ...)                                                                                           \
    if (!(bool)(isFalse))                                                                                              \
    {                                                                                                                  \
        __debugbreak();                                                                                                \
    }
#endif

/**
 * Vector3 를 Vector4 로 확장한다. (w 성분 지정 가능)
 *
 * @param v 입력 3D 벡터.
 * @param w 결과 벡터의 w 성분. 기본값 0.0f (방향 벡터로 취급).
 * @return  (v.x, v.y, v.z, w) 형태의 4D 벡터.
 */
inline DirectX::SimpleMath::Vector4 ToVector4(const DirectX::SimpleMath::Vector3& v, float w = 0.f)
{
    return DirectX::SimpleMath::Vector4(v.x, v.y, v.z, w);
}

/**
 * D3D12 호출 실패 정보를 캡슐화한 예외 클래스.
 * ThrowIfFailed 매크로에 의해 throw 되며, 디버깅에 필요한 호출 위치 정보를 모두 보관한다.
 */
class DxException
{
  public:
    DxException() = default;

    /**
     * 실패한 호출의 컨텍스트로 예외를 구성한다.
     *
     * @param hr           실패한 HRESULT 코드.
     * @param functionName 실패한 함수 또는 표현식의 이름.
     * @param filename     호출이 발생한 소스 파일 경로.
     * @param lineNumber   호출이 발생한 줄 번호.
     */
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    /** 사람이 읽을 수 있는 형태의 오류 메시지로 변환한다. */
    std::wstring ToString() const;

    HRESULT ErrorCode = S_OK;  // 실패 시 반환된 HRESULT 코드.
    std::wstring FunctionName; // 실패한 함수/표현식 이름.
    std::wstring Filename;     // 호출이 발생한 소스 파일.
    int LineNumber = -1;       // 호출이 발생한 줄 번호.
};

/**
 * 그래픽스 관련 유틸리티 함수와 헬퍼 클래스를 모은 네임스페이스.
 * D3D12 리소스/디스크립터 생성처럼 자주 반복되는 보일러플레이트를 캡슐화한다.
 */
namespace GraphicsUtils
{
using namespace std;
typedef unsigned char byte;

/** 파일에서 읽어들인 바이너리 데이터를 공유 소유 형태로 보관하기 위한 타입. */
typedef shared_ptr<vector<byte>> ByteArray;

/**
 * 지정한 파일을 바이너리로 읽어 ByteArray 로 반환한다.
 *
 * @param fileName 읽어들일 파일의 와이드 경로.
 * @return         파일 내용이 담긴 ByteArray. 파일이 없거나 열기 실패 시 빈 ByteArray.
 */
ByteArray ReadFileHelper(const wstring& fileName);

/**
 * D3D12 리소스 생성 보일러플레이트를 캡슐화한 헬퍼 클래스.
 *
 * 디바이스/커맨드 리스트 포인터를 보관해 두고 자주 사용되는
 * 디스크립터 힙, 상수 버퍼, 텍스처, 뷰 생성 함수를 한 줄로 호출할 수 있도록 한다.
 *
 * @note Utility 인스턴스는 디바이스/커맨드 리스트의 소유권을 갖지 않으며,
 *       외부에서 전달된 포인터의 수명은 호출자가 보장해야 한다.
 */
class Utility
{
  public:
    /** 디바이스/커맨드 리스트가 nullptr 인 비활성 상태로 생성한다. */
    Utility();

    /**
     * 사용할 디바이스와 커맨드 리스트를 지정해 생성한다.
     *
     * @param pDevice      D3D12 디바이스 (소유권 없음).
     * @param pCommandList GPU 명령 기록용 커맨드 리스트 (소유권 없음).
     */
    Utility(ID3D12Device5* pDevice);

  private:
    ID3D12Device5* m_device; // 외부 소유 D3D12 디바이스 포인터.

  public:
    /**
     * 지정한 출력 장치에서 사용 가능한 디스플레이 모드 목록을 출력한다. (디버그 용도)
     *
     * @param output 정보를 조회할 IDXGIOutput.
     * @param format 조회할 포맷.
     */
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    /**
     * 디스크립터 힙을 생성한다.
     *
     * @param numDescriptors 힙에 담을 디스크립터 개수.
     * @param type           디스크립터 힙 타입 (CBV_SRV_UAV / RTV / DSV / SAMPLER).
     * @param heap           [out] 생성된 디스크립터 힙이 반환될 ComPtr.
     * @param nodeMask       멀티 GPU 노드 마스크. 단일 GPU 환경에서는 0.
     * @param flag           힙 플래그. 셰이더에서 접근하려면 SHADER_VISIBLE 지정 필요.
     */
    void CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type,
                              Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& heap, UINT nodeMask = 0,
                              D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    /**
     * UPLOAD 힙에 상수 버퍼를 생성하고 CPU 매핑 포인터를 얻는다.
     *
     * @param bufferSize 생성할 버퍼의 바이트 크기. (256 바이트 정렬 권장)
     * @param buffer     [out] 생성된 D3D12 리소스가 반환될 ComPtr.
     * @param pConstant  [out] CPU 측에서 데이터를 기록하기 위한 매핑 포인터.
     *
     * @note Map 호출 후 Unmap 하지 않고 유지하는 영구 매핑(persistent map) 패턴을 사용한다.
     */
    void CreateConstantBuffer(UINT bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, void** pConstant);

    /**
     * 주어진 리소스에 대한 기본 SRV 디스크립터 구조체를 생성한다.
     * (Texture2D, 풀 밉맵 사용 가정)
     *
     * @param resource SRV 를 만들 대상 리소스.
     * @return         리소스 포맷/밉맵 정보가 채워진 SRV 디스크립터.
     */
    D3D12_SHADER_RESOURCE_VIEW_DESC CreateSRVDesc(ID3D12Resource* resource);

    /**
     * DEFAULT 힙에 텍스처 리소스를 생성한다.
     *
     * @param buffer    [out] 생성된 텍스처 리소스.
     * @param width     텍스처 가로 픽셀 수.
     * @param height    텍스처 세로 픽셀 수.
     * @param format    텍스처 포맷.
     * @param flags     리소스 플래그 (RTV/UAV/DSV 허용 등).
     * @param state     초기 리소스 상태.
     * @param mipLevels 생성할 밉맵 레벨 수. 0 이면 풀 밉체인.
     * @param name      디버그 식별을 위한 리소스 이름.
     *
     * @note 깊이 포맷의 경우 클리어 값을 (Depth=1.0, Stencil=0) 으로 설정한다.
     */
    void CreateTextureBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, UINT width, UINT height,
                             DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state,
                             UINT mipLevels, std::wstring name);

    /**
     * 주어진 리소스에 대해 지정한 종류의 뷰(RTV/UAV/SRV/DSV)를 생성한다.
     *
     * @param resource   뷰를 만들 대상 리소스 포인터.
     * @param format   뷰의 포맷.
     * @param bUseMsaa MSAA 를 사용하는지 여부 (UAV 와는 호환되지 않음).
     * @param handle   뷰가 기록될 CPU 디스크립터 핸들.
     * @param type     생성할 뷰의 종류.
     */
    void CreateResourceView(ID3D12Resource* resource, DXGI_FORMAT format, bool bUseMsaa,
                            D3D12_CPU_DESCRIPTOR_HANDLE& handle, const DescriptorType& type,
                            const ViewDimensionType& viewType = ViewDimensionType::TEXTURE2D, UINT miplevel = 0);

    void CreateStructuredResourceView(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, DXGI_FORMAT forma,
                                      D3D12_CPU_DESCRIPTOR_HANDLE& handle, const DescriptorType& type, UINT count,
                                      UINT dataSize);


    template <typename DataType>
    void CreateBuffer(DataType* data, UINT64 bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                      Microsoft::WRL::ComPtr<ID3D12Resource>& upload, D3D12_RESOURCE_FLAGS flag,
                      ID3D12GraphicsCommandList* commandList);

    template <typename DataType>
    void CreateUploadBuffer(DataType* data, UINT64 bufferSize, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                            D3D12_RESOURCE_FLAGS flag, ID3D12GraphicsCommandList* commandList);

    template <typename DataType>
    void CreateBuffer(const std::vector<DataType>& data, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                      Microsoft::WRL::ComPtr<ID3D12Resource>& upload, D3D12_RESOURCE_FLAGS flag,
                      ID3D12GraphicsCommandList* commandList);

    void CreateBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, D3D12_HEAP_TYPE heapType, UINT64 size,
                      D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, std::wstring name);

    std::string MakeTimestamp();

    /**
     * DDS 메모리 블록으로부터 텍스처 리소스를 생성하고 GPU 업로드 명령을 기록한다.
     *
     * 흐름은 CreateBuffer 의 (UPLOAD 힙 생성 → COPY_DEST 전이 → UpdateSubresources → 최종 상태 전이)
     * 과 동일하며, DEFAULT 힙 리소스의 desc 만 DirectXTK 가 DDS 헤더에서 파싱해 채워 준다.
     *
     * @param ddsBytes    DDS 파일 전체 바이트.
     * @param ddsSize     ddsBytes 길이.
     * @param gpu         [out] DEFAULT 힙에 생성된 텍스처 리소스. 종료 상태는 PIXEL_SHADER_RESOURCE.
     * @param upload      [out] 복사용 UPLOAD 힙. 호출자가 ExecuteCommandLists 완료까지 alive 유지.
     * @param loadFlags   sRGB 강제 등 DirectXTK DDS 로더 플래그.
     * @param commandList 업로드 명령을 기록할 그래픽스 커맨드 리스트.
     *
     * @note ddsBytes 메모리도 ExecuteCommandLists 완료까지 호출자가 alive 유지해야 한다
     *       (반환된 subresource 디스크립션이 그 안을 가리키기 때문).
     * @note Compute 등 PS 외 stage 에서 sampling 하려면 호출자가 추가 barrier 를 발행해야 한다.
     */
    void CreateTextureFromDDS(const uint8_t* ddsBytes, size_t ddsSize, Microsoft::WRL::ComPtr<ID3D12Resource>& gpu,
                              Microsoft::WRL::ComPtr<ID3D12Resource>& upload, DirectX::DDS_LOADER_FLAGS loadFlags,
                              ID3D12GraphicsCommandList* commandList);
};
} // namespace GraphicsUtils
#include "Utility.inl"
