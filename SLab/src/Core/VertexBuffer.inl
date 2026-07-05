#pragma once

#include "VertexBuffer.h"

inline VertexBuffer::VertexBuffer()
{
}

inline VertexBuffer::~VertexBuffer()
{
}

template <typename DataType>
inline void VertexBuffer::Initialize(const std::vector<DataType>& data, D3D12_RESOURCE_FLAGS flag,
                                     ID3D12GraphicsCommandList* commandList, std::wstring name)
{
    utility->CreateBuffer<DataType>(data, gpu, upload, flag, commandList);
    bufferSize = UINT(data.size() * sizeof(DataType));
    dataCount = UINT(data.size());
    gpu->SetName(name.c_str());

    vertexBufferView.BufferLocation = gpu->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = (UINT)(data.size() * sizeof(DataType));
    vertexBufferView.StrideInBytes = (UINT)(sizeof(DataType));
}
