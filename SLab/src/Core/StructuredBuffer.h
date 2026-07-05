#pragma once

#include "GPUBuffer.h"

class StructuredBuffer : public GPUBuffer
{
  public:
    StructuredBuffer();
    virtual ~StructuredBuffer();

    template <typename DataType>
    void Initialize(const std::vector<DataType>& data, D3D12_RESOURCE_FLAGS flag,
                    ID3D12GraphicsCommandList* commandList, std::wstring name = L"");

	// upload 전용 버퍼 생성
    template <typename DataType>
    void Initialize(DataType* data, UINT64 size, D3D12_RESOURCE_FLAGS flag, ID3D12GraphicsCommandList* commandList,
                    std::wstring name);
};
#include "StructuredBuffer.inl"
