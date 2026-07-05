#pragma once

#include "GPUBuffer.h"

class VertexBuffer : public GPUBuffer
{
  public:
    VertexBuffer();
    virtual ~VertexBuffer();

    template <typename DataType>
    void Initialize(const std::vector<DataType>& data, D3D12_RESOURCE_FLAGS flag,
                    ID3D12GraphicsCommandList* commandList, std::wstring name = L"");

  public:
    D3D12_VERTEX_BUFFER_VIEW* GetView()
    {
        return &vertexBufferView;
    }

  private:
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};
#include "VertexBuffer.inl"
