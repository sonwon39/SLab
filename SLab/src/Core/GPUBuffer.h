#pragma once

#include "wrl.h"
#include "d3d12.h"
#include <string>
#include <vector>

class GPUBuffer
{

  public:
    GPUBuffer();
    virtual ~GPUBuffer();

    void Initialize(UINT64 bufferSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state,
                    D3D12_RESOURCE_FLAGS flags,
                    std::wstring name);

  public:
    bool Transition(D3D12_RESOURCE_STATES newState, D3D12_RESOURCE_BARRIER& outBarrier);
    // upload 버퍼 clear
    virtual void Clear();
    void Reset();
    void Map();

    void MapForRead();

  public:
    UINT64 bufferSize;
    UINT dataCount;

  public:
    ID3D12Resource* Get() const
    {
        return gpu.Get();
    }
    ID3D12Resource** ReleaseAndGetAddressOf()
    {
        return gpu.ReleaseAndGetAddressOf();
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return gpu->GetGPUVirtualAddress();
    }

  public:
    void SetResourceStates(D3D12_RESOURCE_STATES newState)
    {
        m_currentState = newState;
    }

  public:
    template <typename DataType> void CopyToGpu(const std::vector<DataType>& data);
    template <typename DataType> void CopyToCpu(DataType* data, UINT64 numRows, UINT64 rowSize, UINT64 rowPitch);
    template <typename DataType> void CopyToGpu(DataType* data, UINT64 dataSize);
    template <typename DataType> void CopyToCpu(DataType* data, UINT64 dataSize);

  protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> gpu;
    Microsoft::WRL::ComPtr<ID3D12Resource> upload;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;
    void* pGPU = nullptr;
};
#include "GPUBuffer.inl"
