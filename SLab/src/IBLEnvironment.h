#pragma once

#include "RootSignature.h"
#include "d3d12.h"

class IBLEnvironment
{
  public:
    void Initialize(D3D12_GPU_DESCRIPTOR_HANDLE iblTableBase)
    {
        m_iblTable = iblTableBase;
    }
    void Bind(const RootSignature* rs, ID3D12GraphicsCommandList* cl) const
    {
        if (!rs || !cl)
            return;

        int ibl = rs->GetSlot(BindKey::IBLTable);
        if (ibl >= 0)
            cl->SetGraphicsRootDescriptorTable(ibl, m_iblTable);
    }

  private:
    D3D12_GPU_DESCRIPTOR_HANDLE m_iblTable{0};
};
