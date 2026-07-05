#pragma once

#include "wrl.h"
#include "d3d12.h"
#include <string>
#include "DataType.h"

class DescriptorHeap
{
  public:
    DescriptorHeap();
    virtual ~DescriptorHeap();

  public:
    void Initialize(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT nodeMask,
                    D3D12_DESCRIPTOR_HEAP_FLAGS flag);
    void CreateResourceView(ID3D12Resource* resource, const DescriptorType& descriptorType,
                            const ViewDimensionType& viewDimesionType = ViewDimensionType::TEXTURE2D);

	void Bind(ID3D12GraphicsCommandList* c);

  public:
    // heapIdx 초기화
    void ResetIndex();

  public:
    ID3D12DescriptorHeap* GetHeap() const
    {
        return m_heap.Get();
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int offset) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int offset) const;

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    UINT m_numDescriptors;
    D3D12_DESCRIPTOR_HEAP_TYPE m_descriptorType;

  private:
    UINT m_heapIdx = 0;
    int descriptorIncrementSize = 0;
};
