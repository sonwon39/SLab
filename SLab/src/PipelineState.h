#pragma once

#include "wrl.h"
#include "d3d12.h"
#include "directx/d3dx12.h"

class RootSignature;

class PSO
{
  public:
    PSO(const wchar_t* name) : m_name(name), m_rootSignature(nullptr), m_pso(nullptr)
    {
    }

    void SetRootSignature(const RootSignature& RootSignature)
    {
        m_rootSignature = &RootSignature;
    }
    const RootSignature* GetRootSignature(void) const
    {
        return m_rootSignature;
    }
    ID3D12PipelineState* GetPSO()
    {
        return m_pso.Get();
    }

  protected:
    const wchar_t* m_name;

    const RootSignature* m_rootSignature;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
};

class GraphicsPSO : public PSO
{

  public:
    GraphicsPSO(const wchar_t* name = L"Unnamed Graphics PSO");

    void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
    void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
    void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
    void SetSampleMask(UINT SampleMask);
    void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
    void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
    void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
    void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1,
                                UINT MsaaQuality = 0);
    void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
    // void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

    void SetVertexShader(const void* Binary, size_t Size)
    {
        m_psoDesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
    }
    void SetPixelShader(const void* Binary, size_t Size)
    {
        m_psoDesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
    }
    void SetGeometryShader(const void* Binary, size_t Size)
    {
        m_psoDesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
    }
    void SetHullShader(const void* Binary, size_t Size)
    {
        m_psoDesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
    }
    void SetDomainShader(const void* Binary, size_t Size)
    {
        m_psoDesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
    }

    void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary)
    {
        m_psoDesc.VS = Binary;
    }
    void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary)
    {
        m_psoDesc.PS = Binary;
    }
    void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary)
    {
        m_psoDesc.GS = Binary;
    }
    void SetHullShader(const D3D12_SHADER_BYTECODE& Binary)
    {
        m_psoDesc.HS = Binary;
    }
    void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary)
    {
        m_psoDesc.DS = Binary;
    }

    // RootSignature & InputLayout 초기화 후 PSO 생성
    void Finalize(const Microsoft::WRL::ComPtr<ID3D12Device5>& device);

  private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc{}; // value-init: 설정 안 한 필드를 쓰레기값이 아닌 0으로
    std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC[]> m_inputLayouts;
};

class ComputePSO : public PSO
{
    friend class CommandContext;

  public:
    ComputePSO(const wchar_t* Name = L"Unnamed Compute PSO");

    void SetComputeShader(const void* Binary, size_t Size)
    {
        m_psoDesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
    }
    void SetComputeShader(const D3D12_SHADER_BYTECODE& Binary)
    {
        m_psoDesc.CS = Binary;
    }
    void Finalize(const Microsoft::WRL::ComPtr<ID3D12Device5>& device);

  private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_psoDesc{};
};
