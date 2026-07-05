#include "DescriptorHeap.h"
#include "GraphicsCommon.h"
#include "Engine/World.h"

using namespace Graphics;

DescriptorHeap::DescriptorHeap()
{
}

DescriptorHeap::~DescriptorHeap()
{
}

void DescriptorHeap::Initialize(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT nodeMask,
                                D3D12_DESCRIPTOR_HEAP_FLAGS flag)
{
    m_numDescriptors = numDescriptors;
    m_descriptorType = type;
    if (m_descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        descriptorIncrementSize = m_world->m_cbvSrvDescriptorSize;
    }
    else if (m_descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        descriptorIncrementSize = m_world->m_dsvDescriptorSize;
    }
    else
    {
        descriptorIncrementSize = m_world->m_rtvDescriptorSize;
    }
    utility->CreateDescriptorHeap(numDescriptors, type, m_heap, nodeMask, flag);
}

void DescriptorHeap::CreateResourceView(ID3D12Resource* resource, const DescriptorType& descriptorType,
                                        const ViewDimensionType& viewDimesionType)
{
    DXGI_FORMAT format = resource->GetDesc().Format;
    UINT mipLevel = resource->GetDesc().MipLevels;
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_heapIdx,
                                         descriptorIncrementSize);
    utility->CreateResourceView(resource, format, false, handle, descriptorType, viewDimesionType, mipLevel);
    m_heapIdx++;
}

void DescriptorHeap::Bind(ID3D12GraphicsCommandList* c)
{
    ID3D12DescriptorHeap* heaps[] = {GetHeap()};
    c->SetDescriptorHeaps(1, heaps);
}

void DescriptorHeap::ResetIndex()
{
    m_heapIdx = 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(int offset) const
{
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_heap->GetGPUDescriptorHandleForHeapStart(), offset, descriptorIncrementSize);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(int offset) const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_heap->GetCPUDescriptorHandleForHeapStart(), offset, descriptorIncrementSize);
    return handle;
}
