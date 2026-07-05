#include "PipelineState.h"
#include "RootSignature.h"

GraphicsPSO::GraphicsPSO(const wchar_t* name) : PSO(name)
{
}

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
{
    m_psoDesc.BlendState = BlendDesc;
}

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
{
    m_psoDesc.RasterizerState = RasterizerDesc;
}

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
{
    m_psoDesc.DepthStencilState = DepthStencilDesc;
}

void GraphicsPSO::SetSampleMask(UINT SampleMask)
{
    m_psoDesc.SampleMask = SampleMask;
}

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
    m_psoDesc.PrimitiveTopologyType = TopologyType;
}

void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    m_psoDesc.DSVFormat = DSVFormat;
    m_psoDesc.SampleDesc.Count = MsaaCount;
    m_psoDesc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat,
                                         UINT MsaaCount, UINT MsaaQuality)
{
    for (UINT i = 0; i < NumRTVs; i++)
        m_psoDesc.RTVFormats[i] = RTVFormats[i];

    for (UINT i = NumRTVs; i < m_psoDesc.NumRenderTargets; i++)
        m_psoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

    m_psoDesc.NumRenderTargets = NumRTVs;
    SetDepthTargetFormat(DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{

    m_psoDesc.InputLayout.NumElements = NumElements;

    // pInputElementDescs 메모리 m_inputLayouts로 복사
    if (NumElements > 0)
    {
        size_t size = sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements;
        D3D12_INPUT_ELEMENT_DESC* elements = (D3D12_INPUT_ELEMENT_DESC*)malloc(size);
        memcpy(elements, pInputElementDescs, size);
        m_inputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)elements);
    }
    else
    {
        m_inputLayouts = nullptr;
    }
}

void GraphicsPSO::Finalize(const Microsoft::WRL::ComPtr<ID3D12Device5>& device)
{
    m_psoDesc.pRootSignature = m_rootSignature->GetSignature();
    m_psoDesc.InputLayout.pInputElementDescs = m_inputLayouts.get();
    auto& name = this->m_name;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pso)));
}

ComputePSO::ComputePSO(const wchar_t* name) : PSO(name)
{
}

void ComputePSO::Finalize(const Microsoft::WRL::ComPtr<ID3D12Device5>& device)
{
    m_psoDesc.pRootSignature = m_rootSignature->GetSignature();
    auto& name = this->m_name;

    ThrowIfFailed(device->CreateComputePipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pso)));
}
