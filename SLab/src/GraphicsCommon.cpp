#include "GraphicsCommon.h"
#include "PipelineState.h"
#include "RootSignature.h"

namespace Graphics
{

D3D12_STATIC_SAMPLER_DESC wrapLinearSampler;
D3D12_STATIC_SAMPLER_DESC clampLinearSampler;

D3D12_RASTERIZER_DESC rasterizerDefault;
D3D12_RASTERIZER_DESC wireRasterizer;
D3D12_RASTERIZER_DESC noneCullRasterizer;

D3D12_BLEND_DESC blendNoColorWrite;
D3D12_BLEND_DESC blendColor;

D3D12_DEPTH_STENCIL_DESC depthStateDefault;
D3D12_DEPTH_STENCIL_DESC depthStateCube;

RootSignature g_commonRS;
RootSignature g_defaultRS;
RootSignature g_cubeMapRS;
RootSignature g_PBR_RS;
RootSignature g_SKINNED_RS;

RootSignature g_RenderNoiseParticle_RS;

RootSignature g_U1_RS;
RootSignature g_U1_C1_RS;
RootSignature g_U1_C2_RS;
RootSignature g_U2_C1_RS;
RootSignature g_U2_C2_RS;
RootSignature g_S1_U1_RS;
RootSignature g_S1_U1_C1_RS;
RootSignature g_S1_U3_C1_RS;
RootSignature g_S2_U1_C1_RS;
RootSignature g_S2_U2_C1_RS;

RootSignature g_S1_RS;
RootSignature g_S1_C1_RS;
RootSignature g_SC_RS;
RootSignature g_FLIP_RS;

RootSignature g_SUUC_RS;
RootSignature g_USC_RS;
RootSignature g_UUUC_RS;
RootSignature g_UC_RS;
RootSignature g_UCC_RS;
RootSignature g_UUC_RS;
RootSignature g_UUUSSC_RS;
RootSignature g_UUUUSSC_RS;

RootSignature g_CurlNoiseSimulation_RS;
RootSignature g_NoiseSourcing_RS;

std::shared_ptr<GraphicsUtils::Utility> utility;
std::shared_ptr<World> m_world;
std::shared_ptr<RenderEngine> m_renderEngine;
} // namespace Graphics

void Graphics::InitializeCommonState(const Microsoft::WRL::ComPtr<ID3D12Device5>& device)
{
    wrapLinearSampler = {};
    wrapLinearSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    wrapLinearSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrapLinearSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrapLinearSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrapLinearSampler.MipLODBias = 0;
    wrapLinearSampler.MaxAnisotropy = 0;
    wrapLinearSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    wrapLinearSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    wrapLinearSampler.MinLOD = 0.0f;
    wrapLinearSampler.MaxLOD = D3D12_FLOAT32_MAX;
    wrapLinearSampler.ShaderRegister = 0;
    wrapLinearSampler.RegisterSpace = 0;
    wrapLinearSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    clampLinearSampler = {};
    clampLinearSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    clampLinearSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    clampLinearSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    clampLinearSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    clampLinearSampler.MipLODBias = 0;
    clampLinearSampler.MaxAnisotropy = 0;
    clampLinearSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    clampLinearSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    clampLinearSampler.MinLOD = 0.0f;
    clampLinearSampler.MaxLOD = D3D12_FLOAT32_MAX;
    clampLinearSampler.ShaderRegister = 1;
    clampLinearSampler.RegisterSpace = 0;
    clampLinearSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rasterizerDefault = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    noneCullRasterizer = rasterizerDefault;
    noneCullRasterizer.CullMode = D3D12_CULL_MODE_NONE;
    noneCullRasterizer.DepthClipEnable = FALSE;
    noneCullRasterizer.FrontCounterClockwise = TRUE;

    wireRasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    wireRasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    wireRasterizer.CullMode = D3D12_CULL_MODE_NONE;
    wireRasterizer.DepthClipEnable = false;

    blendNoColorWrite = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendColor = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    blendColor.RenderTarget[0].BlendEnable = true;
    blendColor.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendColor.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    blendColor.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blendColor.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    blendColor.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendColor.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

    depthStateDefault = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    depthStateCube = depthStateDefault;
    depthStateCube.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    g_commonRS.Reset(1, 0);
    // g_commonRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_commonRS[0].InitCBV(0);
    // g_commonRS.InitStaticSampler(0, wrapLinearSampler);
    g_commonRS.Finalize(device, L"CommonRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_U1_RS.Reset(1, 0);
    g_U1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_U1_RS.Finalize(device, L"g_U1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_U1_C1_RS.Reset(2, 0);
    g_U1_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_U1_C1_RS[1].InitCBV(0);
    g_U1_C1_RS.Finalize(device, L"g_U1_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_U1_C2_RS.Reset(3, 0);
    g_U1_C2_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_U1_C2_RS[1].InitCBV(0);
    g_U1_C2_RS[2].InitCBV(1);
    g_U1_C2_RS.Finalize(device, L"g_U1_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_U2_C1_RS.Reset(2, 0);
    g_U2_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    g_U2_C1_RS[1].InitCBV(0);
    g_U2_C1_RS.Finalize(device, L"g_U2_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_U2_C2_RS.Reset(3, 0);
    g_U2_C2_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    g_U2_C2_RS[1].InitCBV(0);
    g_U2_C2_RS[2].InitCBV(1);
    g_U2_C2_RS.Finalize(device, L"g_U2_C2_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_S1_U1_RS.Reset(2, 1);
    g_S1_U1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_S1_U1_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_S1_U1_RS.InitStaticSampler(0, wrapLinearSampler);
    g_S1_U1_RS.Finalize(device, L"g_S1_U1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_S1_U1_C1_RS.Reset(3, 1);
    g_S1_U1_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_S1_U1_C1_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_S1_U1_C1_RS[2].InitCBV(0);
    g_S1_U1_C1_RS.InitStaticSampler(0, wrapLinearSampler);
    g_S1_U1_C1_RS.Finalize(device, L"g_S1_U1_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_S1_U3_C1_RS.Reset(3, 1);
    g_S1_U3_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_S1_U3_C1_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
    g_S1_U3_C1_RS[2].InitCBV(0);
    g_S1_U3_C1_RS.InitStaticSampler(0, wrapLinearSampler);
    g_S1_U3_C1_RS.Finalize(device, L"g_S1_U1_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_S2_U1_C1_RS.Reset(3, 1);
    g_S2_U1_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    g_S2_U1_C1_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_S2_U1_C1_RS[2].InitCBV(0);
    g_S2_U1_C1_RS.InitStaticSampler(0, wrapLinearSampler);
    g_S2_U1_C1_RS.Finalize(device, L"g_S2_U1_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_S2_U2_C1_RS.Reset(3, 1);
    g_S2_U2_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    g_S2_U2_C1_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    g_S2_U2_C1_RS[2].InitCBV(0);
    g_S2_U2_C1_RS.InitStaticSampler(0, wrapLinearSampler);
    g_S2_U2_C1_RS.Finalize(device, L"g_S2_U2_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_PBR_RS.Reset(6, 2);
    g_PBR_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6);
    g_PBR_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 4);
    g_PBR_RS[2].InitCBV(0);
    g_PBR_RS[3].InitCBV(1);
    g_PBR_RS[4].InitCBV(2);
    g_PBR_RS[5].InitSRV(10);

    g_PBR_RS.InitStaticSampler(0, wrapLinearSampler);
    g_PBR_RS.InitStaticSampler(1, clampLinearSampler);
    g_PBR_RS.Finalize(device, L"g_PBR_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    g_PBR_RS.SetSlot(BindKey::MaterialTable, 0);
    g_PBR_RS.SetSlot(BindKey::IBLTable, 1);
    g_PBR_RS.SetSlot(BindKey::GlobalCB, 2);
    g_PBR_RS.SetSlot(BindKey::LocalCB, 3);
    g_PBR_RS.SetSlot(BindKey::MaterialCB, 4);
    g_PBR_RS.SetSlot(BindKey::LightCB, 5);

	g_SKINNED_RS.Reset(7, 2);
    g_SKINNED_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6);
    g_SKINNED_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 4);
    g_SKINNED_RS[2].InitCBV(0);
    g_SKINNED_RS[3].InitCBV(1);
    g_SKINNED_RS[4].InitCBV(2);
    g_SKINNED_RS[5].InitCBV(3);
    g_SKINNED_RS[6].InitSRV(10);
	g_SKINNED_RS.InitStaticSampler(0, wrapLinearSampler);
    g_SKINNED_RS.InitStaticSampler(1, clampLinearSampler);
    g_SKINNED_RS.Finalize(device, L"g_SKINNED_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	g_SKINNED_RS.SetSlot(BindKey::MaterialTable, 0);
    g_SKINNED_RS.SetSlot(BindKey::IBLTable, 1);
    g_SKINNED_RS.SetSlot(BindKey::GlobalCB, 2);
    g_SKINNED_RS.SetSlot(BindKey::LocalCB, 3);
    g_SKINNED_RS.SetSlot(BindKey::MaterialCB, 4);
    g_SKINNED_RS.SetSlot(BindKey::SkinnedMeshCB, 5);
    g_SKINNED_RS.SetSlot(BindKey::LightCB, 6);

    g_RenderNoiseParticle_RS.Reset(2, 0);
    g_RenderNoiseParticle_RS[0].InitSRV(0);
    g_RenderNoiseParticle_RS[1].InitCBV(0);
    g_RenderNoiseParticle_RS.Finalize(device, L"g_RenderNoiseParticle_RS",
                                      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    g_RenderNoiseParticle_RS.SetSlot(BindKey::GlobalCB, 1);

    g_S1_RS.Reset(1, 0);
    g_S1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_S1_RS.Finalize(device, L"g_S1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_S1_C1_RS.Reset(2, 1);
    g_S1_C1_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_S1_C1_RS[1].InitCBV(0);
    g_S1_C1_RS.InitStaticSampler(0, wrapLinearSampler);
    g_S1_C1_RS.Finalize(device, L"g_S1_C1_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_SC_RS.Reset(2, 1);
    g_SC_RS[0].InitSRV(0);
    g_SC_RS[1].InitCBV(0);
    g_SC_RS.InitStaticSampler(0, wrapLinearSampler);
    g_SC_RS.Finalize(device, L"g_SC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // FLIP screen-space fluid: one root sig for every render pass.
    //   [0] CBV b0 frame, [1] 32-bit consts b1 (per-pass), [2] SRV t4 particles (VS),
    //   [3..5] SRV tables t0/t1/t2 (PS textures), s0 linear sampler.
    g_FLIP_RS.Reset(6, 1);
    g_FLIP_RS[0].InitCBV(0);
    g_FLIP_RS[1].InitConstants(4, 1);
    g_FLIP_RS[2].InitSRV(4, D3D12_SHADER_VISIBILITY_VERTEX);
    g_FLIP_RS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    g_FLIP_RS[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    g_FLIP_RS[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    g_FLIP_RS.InitStaticSampler(0, wrapLinearSampler);
    g_FLIP_RS.Finalize(device, L"g_FLIP_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_defaultRS.Reset(3, 1);
    g_defaultRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_defaultRS[1].InitCBV(0);
    g_defaultRS[2].InitCBV(1);
    g_defaultRS.InitStaticSampler(0, wrapLinearSampler);
    g_defaultRS.Finalize(device, L"g_defaultRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    g_defaultRS.SetSlot(BindKey::MaterialTable, 0);
    g_defaultRS.SetSlot(BindKey::GlobalCB, 1);
    g_defaultRS.SetSlot(BindKey::LocalCB, 2);

    g_cubeMapRS.Reset(3, 1);
    g_cubeMapRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_cubeMapRS[1].InitCBV(0);
    g_cubeMapRS[2].InitCBV(1);
    g_cubeMapRS.InitStaticSampler(0, wrapLinearSampler);
    g_cubeMapRS.Finalize(device, L"g_cubeMapRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    g_cubeMapRS.SetSlot(BindKey::MaterialTable, 0);
    g_cubeMapRS.SetSlot(BindKey::GlobalCB, 1);
    g_cubeMapRS.SetSlot(BindKey::LocalCB, 2);

    g_SUUC_RS.Reset(4, 0);
    g_SUUC_RS[0].InitSRV(0);
    g_SUUC_RS[1].InitUAV(0);
    g_SUUC_RS[2].InitUAV(1);
    g_SUUC_RS[3].InitCBV(0);
    g_SUUC_RS.Finalize(device, L"g_SUUC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_USC_RS.Reset(3, 0);
    g_USC_RS[0].InitUAV(0);
    g_USC_RS[1].InitSRV(0);
    g_USC_RS[2].InitCBV(0);
    g_USC_RS.Finalize(device, L"g_USC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_UUUC_RS.Reset(4, 0);
    g_UUUC_RS[0].InitUAV(0);
    g_UUUC_RS[1].InitUAV(1);
    g_UUUC_RS[2].InitUAV(2);
    g_UUUC_RS[3].InitCBV(0);
    g_UUUC_RS.Finalize(device, L"g_UUUC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_UC_RS.Reset(2, 0);
    g_UC_RS[0].InitUAV(0);
    g_UC_RS[1].InitCBV(0);
    g_UC_RS.Finalize(device, L"g_UC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_UCC_RS.Reset(3, 0);
    g_UCC_RS[0].InitUAV(0);
    g_UCC_RS[1].InitCBV(0);
    g_UCC_RS[2].InitCBV(1);
    g_UCC_RS.Finalize(device, L"g_UCC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_UUC_RS.Reset(3, 0);
    g_UUC_RS[0].InitUAV(0);
    g_UUC_RS[1].InitUAV(1);
    g_UUC_RS[2].InitCBV(0);
    g_UUC_RS.Finalize(device, L"g_UUC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_UUUSSC_RS.Reset(6, 0);
    g_UUUSSC_RS[0].InitUAV(0);
    g_UUUSSC_RS[1].InitUAV(1);
    g_UUUSSC_RS[2].InitUAV(2);
    g_UUUSSC_RS[3].InitSRV(0);
    g_UUUSSC_RS[4].InitSRV(1);
    g_UUUSSC_RS[5].InitCBV(0);
    g_UUUSSC_RS.Finalize(device, L"g_UUUSSC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_UUUUSSC_RS.Reset(7, 0);
    g_UUUUSSC_RS[0].InitUAV(0);
    g_UUUUSSC_RS[1].InitUAV(1);
    g_UUUUSSC_RS[2].InitUAV(2);
    g_UUUUSSC_RS[3].InitUAV(3);
    g_UUUUSSC_RS[4].InitSRV(0);
    g_UUUUSSC_RS[5].InitSRV(1);
    g_UUUUSSC_RS[6].InitCBV(0);
    g_UUUUSSC_RS.Finalize(device, L"g_UUUUSSC_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_CurlNoiseSimulation_RS.Reset(2, 1);
    g_CurlNoiseSimulation_RS[0].InitUAV(0);
    g_CurlNoiseSimulation_RS[1].InitCBV(0);
    g_CurlNoiseSimulation_RS.InitStaticSampler(0, wrapLinearSampler);
    g_CurlNoiseSimulation_RS.Finalize(device, L"g_CurlNoiseSimulation_RS",
                                      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    g_NoiseSourcing_RS.Reset(3, 1);
    g_NoiseSourcing_RS[0].InitSRV(0);
    g_NoiseSourcing_RS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_NoiseSourcing_RS[2].InitCBV(0);
    g_NoiseSourcing_RS.InitStaticSampler(0, wrapLinearSampler);
    g_NoiseSourcing_RS.Finalize(device, L"g_NoiseSourcing_RS",
                                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}
