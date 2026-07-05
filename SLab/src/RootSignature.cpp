#include "RootSignature.h"
#include <iostream>

using Microsoft::WRL::ComPtr;

void RootSignature::InitStaticSampler(UINT index, const D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc,
                                      D3D12_SHADER_VISIBILITY visibility)
{
#if defined(DEBUG) || (_DEBUG)
    ASSERT(index < m_numSamplers);
#endif

    m_samplerArray[index] = staticSamplerDesc;
}

void RootSignature::Finalize(const ComPtr<ID3D12Device5>& device, const std::wstring& name,
                             D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC m_rootSignatureDesc;

    m_rootSignatureDesc.Init_1_0(m_numParameters, (const D3D12_ROOT_PARAMETER*)m_paramArray.get(), m_numSamplers,
                                 (const D3D12_STATIC_SAMPLER_DESC*)m_samplerArray.get(), flags);

    ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

    HRESULT hr =
        D3D12SerializeVersionedRootSignature(&m_rootSignatureDesc, pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);
    }

    ThrowIfFailed(device->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
                                              IID_PPV_ARGS(&m_signature)));

    m_signature->SetName(name.c_str());
}

int RootSignature::GetSlot(BindKey k) const
{
    if (m_slotOf.size() > (uint32_t)k)
        return m_slotOf[(int)k];
    return -1;
}
