#pragma once

#include "GPUBuffer.h"

class IndexBuffer : public GPUBuffer
{
  public:
    IndexBuffer();
    virtual ~IndexBuffer();

    template <typename DataType>
    void Initialize(const std::vector<DataType>& data, D3D12_RESOURCE_FLAGS flag,
                    ID3D12GraphicsCommandList* commandList, std::wstring name = L"");

  public:
    D3D12_INDEX_BUFFER_VIEW* GetView()
    {
        return &indexBufferView;
    }

  private:
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
};
#include "IndexBuffer.inl"
