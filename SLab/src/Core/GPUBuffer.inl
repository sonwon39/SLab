#pragma once

#include "GPUBuffer.h"
#include "GraphicsCommon.h"
#include "Utility.h"

using namespace Graphics;

inline GPUBuffer::GPUBuffer()
{
}

inline GPUBuffer::~GPUBuffer()
{
    Clear();
}

inline void GPUBuffer::Initialize(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state,
                                  D3D12_RESOURCE_FLAGS flags, std::wstring name)
{
    bufferSize = size;
    utility->CreateBuffer(gpu, heapType, bufferSize, flags, state, name);
}

inline void GPUBuffer::Clear()
{
    if (upload)
        upload.Reset();
}

inline void GPUBuffer::Reset()
{
    if (gpu)
        gpu.Reset();
}

inline void GPUBuffer::Map()
{
    CD3DX12_RANGE range(0, 0);
    ThrowIfFailed(gpu->Map(0, &range, static_cast<void**>(&pGPU)));
}

inline void GPUBuffer::MapForRead()
{
    CD3DX12_RANGE range(0, bufferSize);
    if (pGPU)
        return;
    ThrowIfFailed(gpu->Map(0, &range, static_cast<void**>(&pGPU)));
}

inline bool GPUBuffer::Transition(D3D12_RESOURCE_STATES newState, D3D12_RESOURCE_BARRIER& outBarrier)
{
    if (m_currentState == newState)
    {
        if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {

            outBarrier = CD3DX12_RESOURCE_BARRIER::UAV(gpu.Get());
            return true;
        }

        return false;
    }

    outBarrier = CD3DX12_RESOURCE_BARRIER::Transition(gpu.Get(), m_currentState, newState);

    m_currentState = newState;
    return true;
}
template <typename DataType> inline void GPUBuffer::CopyToGpu(DataType* data, UINT64 dataSize)
{
    if (!pGPU)
        return;

    if (dataSize > bufferSize)
        return;

    if (pGPU)
    {
        memcpy(pGPU, data, dataSize);
    }
}
template <typename DataType> inline void GPUBuffer::CopyToGpu(const std::vector<DataType>& data)
{
    if (!pGPU)
        return;

    UINT dataSize = data.size() * sizeof(DataType);
    if (dataSize > bufferSize)
        return;

    if (pGPU)
    {
        memcpy(pGPU, data.data(), dataSize);
    }
}
template <typename DataType>
inline void GPUBuffer::CopyToCpu(DataType* data, UINT64 numRows, UINT64 rowSize, UINT64 rowPitch)
{
    if (!pGPU)
        return;
    // rawSize  유효데이터 
    // rawPitch 256배수 실제 raw 크기
    auto* dst = reinterpret_cast<uint8_t*>(data);
    const auto* src = reinterpret_cast<const uint8_t*>(pGPU);
    for (UINT y = 0; y < numRows; y++)
    {
        memcpy(dst + y * rowSize, src + y * rowPitch, rowSize);
    }
}
template <typename DataType> inline void GPUBuffer::CopyToCpu(DataType* data, UINT64 dataSize)
{
    if (!pGPU)
        return;

    if (dataSize > bufferSize)
    {
        std::cout << "buffer 크기보다 dataSize가 더 큽니다\n";
        return;
    }

    if (pGPU)
    {
        memcpy(data, pGPU, dataSize);
    }
}
