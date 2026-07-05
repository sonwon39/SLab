#pragma once

#include "StructuredBuffer.h"


inline StructuredBuffer::StructuredBuffer()
{
}
inline StructuredBuffer::~StructuredBuffer()
{
}

template <typename DataType>
inline void StructuredBuffer::Initialize(const std::vector<DataType>& data, D3D12_RESOURCE_FLAGS flag,
                                    ID3D12GraphicsCommandList* commandList, std::wstring name)
{
    utility->CreateBuffer<DataType>(data, gpu, upload, flag, commandList);
    bufferSize = UINT64(data.size() * sizeof(DataType));
    dataCount = UINT(data.size());
    gpu->SetName(name.c_str());
}
template <typename DataType>
inline void StructuredBuffer::Initialize(DataType* data, UINT64 size, D3D12_RESOURCE_FLAGS flag,
                                         ID3D12GraphicsCommandList* commandList, std::wstring name)
{
    bufferSize = size;

    utility->CreateUploadBuffer<DataType>(data, bufferSize, gpu, flag, commandList);
    dataCount = UINT(bufferSize / sizeof(DataType));
    gpu->SetName(name.c_str());
}


