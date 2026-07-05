#pragma once

#include "wrl.h"
#include "d3d12.h"

template <typename Constant> class ConstantBuffer
{
  public:
    ConstantBuffer() {};
    virtual ~ConstantBuffer() {};

  public:
    void Initialize(const Constant& init);
    void Update();

  public:
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
    {
        return m_localCB->GetGPUVirtualAddress();
    }

  public:
    Constant localConstant;

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_localCB;
    void* pLocalCB;
};
#include "ConstantBuffer.inl"
