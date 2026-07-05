#pragma once

#include <mutex>
#include <atomic>
#include "Utility.h"
#include "imgui.h"

class SamplerDesc;
class RootSignature;
class ComputePSO;
class GraphicsPSO;
class World;
class RenderEngine;

namespace Graphics
{
// sampler, rootsignature 등을 미리 생성
void InitializeCommonState(const Microsoft::WRL::ComPtr<ID3D12Device5>& device);

extern D3D12_STATIC_SAMPLER_DESC wrapLinearSampler;

extern D3D12_RASTERIZER_DESC rasterizerDefault;
extern D3D12_RASTERIZER_DESC wireRasterizer;
extern D3D12_RASTERIZER_DESC noneCullRasterizer;

extern D3D12_BLEND_DESC blendNoColorWrite;
extern D3D12_BLEND_DESC blendColor;

extern D3D12_DEPTH_STENCIL_DESC depthStateDefault;
extern D3D12_DEPTH_STENCIL_DESC depthStateCube;

extern RootSignature g_commonRS;
extern RootSignature g_defaultRS;
extern RootSignature g_cubeMapRS;
extern RootSignature g_PBR_RS;
extern RootSignature g_SKINNED_RS;
extern RootSignature g_RenderNoiseParticle_RS;

extern RootSignature g_U1_RS;
extern RootSignature g_U1_C1_RS;
extern RootSignature g_U1_C2_RS;
extern RootSignature g_U2_C1_RS;
extern RootSignature g_U2_C2_RS;
extern RootSignature g_S1_U1_C1_RS;
extern RootSignature g_S1_U1_RS;

extern RootSignature g_S1_U3_C1_RS;
extern RootSignature g_S2_U1_C1_RS;
extern RootSignature g_S2_U2_C1_RS;

extern RootSignature g_S1_RS;
extern RootSignature g_S1_C1_RS;
extern RootSignature g_SC_RS;
extern RootSignature g_FLIP_RS; // FLIP SSF: CBV b0, consts b1, SRV t4(VS), tables t0/t1/t2(PS), s0

extern RootSignature g_SUUC_RS;
extern RootSignature g_USC_RS;
extern RootSignature g_UUUC_RS;
extern RootSignature g_UC_RS;
extern RootSignature g_UCC_RS;
extern RootSignature g_UUC_RS;
extern RootSignature g_UUUSSC_RS;
extern RootSignature g_UUUUSSC_RS;

extern RootSignature g_CurlNoiseSimulation_RS;
extern RootSignature g_NoiseSourcing_RS;

extern std::shared_ptr<GraphicsUtils::Utility> utility;
extern std::shared_ptr<World> m_world;
extern std::shared_ptr<RenderEngine> m_renderEngine;

} // namespace Graphics
