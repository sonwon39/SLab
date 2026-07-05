#include "Renderer.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "CompiledShaders/DefaultVS.h"
#include "CompiledShaders/DefaultPS.h"
#include "CompiledShaders/DefaultCS.h"

#include "CompiledShaders/CubeMapVS.h"
#include "CompiledShaders/CubeMapPS.h"

#include "CompiledShaders/SkinnedMeshVS.h"
#include "CompiledShaders/PBRVS.h"
#include "CompiledShaders/PBRPS.h"

#include "CompiledShaders/ParticleRenderVS.h"
#include "CompiledShaders/ParticleRenderGS.h"
#include "CompiledShaders/ParticleRenderPS.h"
#include "CompiledShaders/ParticleSimulationCS.h"

#include "CompiledShaders/ComputeDensityCS.h"
#include "CompiledShaders/ComputeForcesCS.h"
#include "CompiledShaders/ComputePressureCS.h"
#include "CompiledShaders/SPHSimulationCS.h"

#include "CompiledShaders/ClearCounterCS.h"
#include "CompiledShaders/CountingCS.h"
#include "CompiledShaders/ScanBlocksCS.h"
#include "CompiledShaders/AddBlockOffsetsCS.h"
#include "CompiledShaders/ScatterCS.h"

#include "CompiledShaders/SourcingCS.h"
#include "CompiledShaders/ComputeCurlCS.h"
#include "CompiledShaders/VorticityConfinementCS.h"
#include "CompiledShaders/AdvectionCS.h"
#include "CompiledShaders/ComputeDivergenceCS.h"
#include "CompiledShaders/JacobiCS.h"
#include "CompiledShaders/ComputeFinalVelocityCS.h"

#include "CompiledShaders/PerlinNoiseCS.h"
#include "CompiledShaders/CurlNoiseCS.h"
#include "CompiledShaders/CurlNoiseSimulationCS.h"

#include "CompiledShaders/NoiseParticleRenderVS.h"
#include "CompiledShaders/NoiseParticleRenderGS.h"
#include "CompiledShaders/NoiseParticleRenderPS.h"
#include "CompiledShaders/NoiseSourcingCS.h"

// flip water (build-along)
#include "CompiledShaders/FlipBallisticCS.h"
#include "CompiledShaders/FlipParticleVS.h"
#include "CompiledShaders/FlipParticlePS.h"
#include "CompiledShaders/FlipBackgroundVS.h"
#include "CompiledShaders/FlipBackgroundPS.h"
#include "CompiledShaders/FlipDepthPS.h"
#include "CompiledShaders/FlipThickPS.h"
#include "CompiledShaders/FlipSmoothPS.h"
#include "CompiledShaders/FlipCompositePS.h"

using namespace Graphics;
using namespace Renderer;

namespace Renderer
{
std::map<std::string, GraphicsPSO> m_PSOs;
std::map<std::string, ComputePSO> m_CPSOs;

std::vector<std::string> psoNames;
std::vector<std::string> cpsoNames;

DXGI_FORMAT hdrFormat;
DXGI_FORMAT backBufferFormat;
DXGI_FORMAT dsBufferFormat;
DXGI_FORMAT dsOnlyFormat;
DXGI_FORMAT dsOnlyDsvFormat;
DXGI_FORMAT dsOnlySrvFormat;

} // namespace Renderer

bool Renderer::GetGraphicsPSO(const std::string& psoName, GraphicsPSO& pso)
{
    if (m_PSOs.find(psoName) != m_PSOs.end())
    {
        pso = m_PSOs[psoName];
        return true;
    }
    else
    {
        return false;
    }
}
ComputePSO Renderer::GetComputePSO(const std::string& psoName)
{
    ComputePSO pso;
    if (m_CPSOs.find(psoName) != m_CPSOs.end())
    {
        pso = m_CPSOs[psoName];
    }
    else
    {
        pso = m_CPSOs["defaultCPSO"];
    }
    return pso;
}
void Renderer::BindCPSO(const std::string& psoName, ID3D12GraphicsCommandList* c)
{
    ComputePSO cpso = GetComputePSO(psoName);
    c->SetPipelineState(cpso.GetPSO());
    c->SetComputeRootSignature(cpso.GetRootSignature()->GetSignature());
}
void Renderer::BindPSO(const std::string& psoName, ID3D12GraphicsCommandList* c)
{
    GraphicsPSO pso;
	if (GetGraphicsPSO(psoName, pso))
	{
        c->SetPipelineState(pso.GetPSO());
        c->SetGraphicsRootSignature(pso.GetRootSignature()->GetSignature());
	}
}
void Renderer::Initialize(const Microsoft::WRL::ComPtr<ID3D12Device5>& device)
{
    GraphicsPSO defaultPSO(L"default PSO");
    GraphicsPSO cubeMapPSO(L"cubeMap PSO");
    GraphicsPSO pbrPSO(L"pbr PSO");
    GraphicsPSO skinnedPSO(L"skinned PSO");

    GraphicsPSO particleRenderPSO(L"particleRender PSO");
    GraphicsPSO noiseParticleRenderPSO(L"noiseParticleRender PSO");
    GraphicsPSO flipParticleRenderPSO(L"flipParticleRender PSO");
    GraphicsPSO flipBgPSO(L"flipBg PSO");
    GraphicsPSO flipDepthPSO(L"flipDepth PSO");
    GraphicsPSO flipThickPSO(L"flipThick PSO");
    GraphicsPSO flipSmoothPSO(L"flipSmooth PSO");
    GraphicsPSO flipBlurPSO(L"flipBlur PSO");
    GraphicsPSO flipCompositePSO(L"flipComposite PSO");

    hdrFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    // backBufferFormat  = hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    dsBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsOnlyFormat = DXGI_FORMAT_R32_TYPELESS;
    dsOnlyDsvFormat = DXGI_FORMAT_D32_FLOAT;
    dsOnlySrvFormat = DXGI_FORMAT_R32_FLOAT;

    D3D12_INPUT_ELEMENT_DESC posOnlyIL[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                                             D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                             0}};

    D3D12_INPUT_ELEMENT_DESC simpleIL[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                           {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}

    };
    D3D12_INPUT_ELEMENT_DESC textIL[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                         {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                         {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_INPUT_ELEMENT_DESC phongIL[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                          {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                          {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_INPUT_ELEMENT_DESC pbrIL[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_INPUT_ELEMENT_DESC skinnedMeshIL[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 1, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}

    };
    D3D12_INPUT_ELEMENT_DESC pointCloudIL[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    defaultPSO.SetInputLayout(_countof(phongIL), phongIL);
    defaultPSO.SetRootSignature(g_defaultRS);
    defaultPSO.SetRasterizerState(rasterizerDefault);
    defaultPSO.SetBlendState(blendNoColorWrite);
    defaultPSO.SetDepthStencilState(depthStateDefault); // DepthEnable=TRUE, Func=LESS
    defaultPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    defaultPSO.SetVertexShader(g_pDefaultVS, sizeof(g_pDefaultVS));
    defaultPSO.SetPixelShader(g_pDefaultPS, sizeof(g_pDefaultPS));
    defaultPSO.SetSampleMask(UINT_MAX);
    defaultPSO.SetRenderTargetFormat(backBufferFormat, dsBufferFormat, 1, 0);
    defaultPSO.Finalize(device);
    m_PSOs["defaultPSO"] = defaultPSO;
    psoNames.push_back("defaultPSO");

    cubeMapPSO.SetInputLayout(_countof(simpleIL), simpleIL);
    cubeMapPSO.SetRootSignature(g_cubeMapRS);
    cubeMapPSO.SetRasterizerState(noneCullRasterizer);
    cubeMapPSO.SetBlendState(blendNoColorWrite);
    cubeMapPSO.SetDepthStencilState(depthStateCube); // DepthEnable=TRUE, Func=LESS_EQUAL
    cubeMapPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    cubeMapPSO.SetVertexShader(g_pCubeMapVS, sizeof(g_pCubeMapVS));
    cubeMapPSO.SetPixelShader(g_pCubeMapPS, sizeof(g_pCubeMapPS));
    cubeMapPSO.SetSampleMask(UINT_MAX);
    cubeMapPSO.SetRenderTargetFormat(backBufferFormat, dsBufferFormat, 1, 0);
    cubeMapPSO.Finalize(device);
    m_PSOs["cubeMapPSO"] = cubeMapPSO;
    psoNames.push_back("cubeMapPSO");

    pbrPSO.SetInputLayout(_countof(pbrIL), pbrIL);
    pbrPSO.SetRootSignature(g_PBR_RS);
    pbrPSO.SetRasterizerState(rasterizerDefault);
    pbrPSO.SetBlendState(blendNoColorWrite);
    pbrPSO.SetDepthStencilState(depthStateDefault); // DepthEnable=TRUE, Func=LESS_EQUAL
    pbrPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    pbrPSO.SetVertexShader(g_pPBRVS, sizeof(g_pPBRVS));
    pbrPSO.SetPixelShader(g_pPBRPS, sizeof(g_pPBRPS));
    pbrPSO.SetSampleMask(UINT_MAX);
    pbrPSO.SetRenderTargetFormat(backBufferFormat, dsBufferFormat, 1, 0);
    pbrPSO.Finalize(device);
    m_PSOs["pbrPSO"] = pbrPSO;
    psoNames.push_back("pbrPSO");

	skinnedPSO.SetInputLayout(_countof(skinnedMeshIL), skinnedMeshIL);
    skinnedPSO.SetRootSignature(g_SKINNED_RS);
    skinnedPSO.SetRasterizerState(rasterizerDefault);
    skinnedPSO.SetBlendState(blendNoColorWrite);
    skinnedPSO.SetDepthStencilState(depthStateDefault); // DepthEnable=TRUE, Func=LESS_EQUAL
    skinnedPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    skinnedPSO.SetVertexShader(g_pSkinnedMeshVS, sizeof(g_pSkinnedMeshVS));
    skinnedPSO.SetPixelShader(g_pPBRPS, sizeof(g_pPBRPS));
    skinnedPSO.SetSampleMask(UINT_MAX);
    skinnedPSO.SetRenderTargetFormat(backBufferFormat, dsBufferFormat, 1, 0);
    skinnedPSO.Finalize(device);
    m_PSOs["skinnedPSO"] = skinnedPSO;
    psoNames.push_back("skinnedPSO");

    particleRenderPSO.SetInputLayout(0, nullptr);
    particleRenderPSO.SetRootSignature(g_defaultRS);
    particleRenderPSO.SetRasterizerState(rasterizerDefault);
    particleRenderPSO.SetBlendState(blendColor);
    particleRenderPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
    particleRenderPSO.SetVertexShader(g_pParticleRenderVS, sizeof(g_pParticleRenderVS));
    particleRenderPSO.SetGeometryShader(g_pParticleRenderGS, sizeof(g_pParticleRenderGS));
    particleRenderPSO.SetPixelShader(g_pParticleRenderPS, sizeof(g_pParticleRenderPS));
    particleRenderPSO.SetSampleMask(UINT_MAX);
    particleRenderPSO.SetRenderTargetFormat(backBufferFormat, DXGI_FORMAT_UNKNOWN, 1, 0);
    particleRenderPSO.Finalize(device);
    m_PSOs["particleRenderPSO"] = particleRenderPSO;
    psoNames.push_back("particleRenderPSO");

	noiseParticleRenderPSO.SetInputLayout(0, nullptr);
    noiseParticleRenderPSO.SetRootSignature(g_RenderNoiseParticle_RS);
    noiseParticleRenderPSO.SetRasterizerState(rasterizerDefault);
    noiseParticleRenderPSO.SetBlendState(blendColor);
    noiseParticleRenderPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
    noiseParticleRenderPSO.SetVertexShader(g_pNoiseParticleRenderVS, sizeof(g_pNoiseParticleRenderVS));
    noiseParticleRenderPSO.SetGeometryShader(g_pNoiseParticleRenderGS, sizeof(g_pNoiseParticleRenderGS));
    noiseParticleRenderPSO.SetPixelShader(g_pNoiseParticleRenderPS, sizeof(g_pNoiseParticleRenderPS));
    noiseParticleRenderPSO.SetSampleMask(UINT_MAX);
    noiseParticleRenderPSO.SetRenderTargetFormat(backBufferFormat, DXGI_FORMAT_UNKNOWN, 1, 0);
    noiseParticleRenderPSO.Finalize(device);
    m_PSOs["noiseParticleRenderPSO"] = noiseParticleRenderPSO;
    psoNames.push_back("noiseParticleRenderPSO");

    // ---- FLIP screen-space fluid (build-along) : all passes share g_FLIP_RS ----
    // offscreen depth target is D32 (separate from the D24S8 main depth); billboards
    // are camera-facing so every FLIP pass disables back-face culling.
    const DXGI_FORMAT SSF_SCENE = DXGI_FORMAT_R16G16B16A16_FLOAT; // rgb + view-depth in a
    const DXGI_FORMAT SSF_DEPTH = DXGI_FORMAT_R32_FLOAT;          // linear view depth
    const DXGI_FORMAT SSF_THICK = DXGI_FORMAT_R16_FLOAT;          // accumulated thickness
    const DXGI_FORMAT SSF_D32 = DXGI_FORMAT_D32_FLOAT;            // hardware depth (nearest)

    // debug particles (view mode 1): re-uses the D32 from the depth pass with an
    // EQUAL test (same depth math -> only the nearest sphere shades) and no write.
    D3D12_DEPTH_STENCIL_DESC depthEqualNoWrite = depthStateDefault;
    depthEqualNoWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthEqualNoWrite.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    flipParticleRenderPSO.SetInputLayout(0, nullptr);
    flipParticleRenderPSO.SetRootSignature(g_FLIP_RS);
    flipParticleRenderPSO.SetRasterizerState(noneCullRasterizer);
    flipParticleRenderPSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    flipParticleRenderPSO.SetDepthStencilState(depthEqualNoWrite);
    flipParticleRenderPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipParticleRenderPSO.SetVertexShader(g_pFlipParticleVS, sizeof(g_pFlipParticleVS));
    flipParticleRenderPSO.SetPixelShader(g_pFlipParticlePS, sizeof(g_pFlipParticlePS));
    flipParticleRenderPSO.SetSampleMask(UINT_MAX);
    flipParticleRenderPSO.SetRenderTargetFormat(backBufferFormat, SSF_D32, 1, 0);
    flipParticleRenderPSO.Finalize(device);
    m_PSOs["flipParticleRenderPSO"] = flipParticleRenderPSO;
    psoNames.push_back("flipParticleRenderPSO");

    // background: pool-room raytrace -> sceneColor (rgb linear + view depth in alpha)
    flipBgPSO.SetInputLayout(0, nullptr);
    flipBgPSO.SetRootSignature(g_FLIP_RS);
    flipBgPSO.SetRasterizerState(noneCullRasterizer);
    flipBgPSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    flipBgPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipBgPSO.SetVertexShader(g_pFlipBackgroundVS, sizeof(g_pFlipBackgroundVS));
    flipBgPSO.SetPixelShader(g_pFlipBackgroundPS, sizeof(g_pFlipBackgroundPS));
    flipBgPSO.SetSampleMask(UINT_MAX);
    flipBgPSO.SetRenderTargetFormat(SSF_SCENE, DXGI_FORMAT_UNKNOWN, 1, 0);
    flipBgPSO.Finalize(device);
    m_PSOs["flipBgPSO"] = flipBgPSO;
    psoNames.push_back("flipBgPSO");

    // depth: billboard sphere front-face linear depth -> depthRaw (R32F) + D32
    flipDepthPSO.SetInputLayout(0, nullptr);
    flipDepthPSO.SetRootSignature(g_FLIP_RS);
    flipDepthPSO.SetRasterizerState(noneCullRasterizer);
    flipDepthPSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    flipDepthPSO.SetDepthStencilState(depthStateDefault); // LESS + write (nearest)
    flipDepthPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipDepthPSO.SetVertexShader(g_pFlipParticleVS, sizeof(g_pFlipParticleVS));
    flipDepthPSO.SetPixelShader(g_pFlipDepthPS, sizeof(g_pFlipDepthPS));
    flipDepthPSO.SetSampleMask(UINT_MAX);
    flipDepthPSO.SetRenderTargetFormat(SSF_DEPTH, SSF_D32, 1, 0);
    flipDepthPSO.Finalize(device);
    m_PSOs["flipDepthPSO"] = flipDepthPSO;
    psoNames.push_back("flipDepthPSO");

    // thickness: additive view-ray chord length -> thickness (R16F), no depth
    flipThickPSO.SetInputLayout(0, nullptr);
    flipThickPSO.SetRootSignature(g_FLIP_RS);
    flipThickPSO.SetRasterizerState(noneCullRasterizer);
    flipThickPSO.SetBlendState(blendColor); // additive (ONE, ONE)
    flipThickPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipThickPSO.SetVertexShader(g_pFlipParticleVS, sizeof(g_pFlipParticleVS));
    flipThickPSO.SetPixelShader(g_pFlipThickPS, sizeof(g_pFlipThickPS));
    flipThickPSO.SetSampleMask(UINT_MAX);
    flipThickPSO.SetRenderTargetFormat(SSF_THICK, DXGI_FORMAT_UNKNOWN, 1, 0);
    flipThickPSO.Finalize(device);
    m_PSOs["flipThickPSO"] = flipThickPSO;
    psoNames.push_back("flipThickPSO");

    // narrow-range depth smooth (fullscreen) -> R32F ping-pong
    flipSmoothPSO.SetInputLayout(0, nullptr);
    flipSmoothPSO.SetRootSignature(g_FLIP_RS);
    flipSmoothPSO.SetRasterizerState(noneCullRasterizer);
    flipSmoothPSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    flipSmoothPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipSmoothPSO.SetVertexShader(g_pFlipBackgroundVS, sizeof(g_pFlipBackgroundVS));
    flipSmoothPSO.SetPixelShader(g_pFlipSmoothPS, sizeof(g_pFlipSmoothPS));
    flipSmoothPSO.SetSampleMask(UINT_MAX);
    flipSmoothPSO.SetRenderTargetFormat(SSF_DEPTH, DXGI_FORMAT_UNKNOWN, 1, 0);
    flipSmoothPSO.Finalize(device);
    m_PSOs["flipSmoothPSO"] = flipSmoothPSO;
    psoNames.push_back("flipSmoothPSO");

    // thickness gaussian blur: same shader, R16F target
    flipBlurPSO.SetInputLayout(0, nullptr);
    flipBlurPSO.SetRootSignature(g_FLIP_RS);
    flipBlurPSO.SetRasterizerState(noneCullRasterizer);
    flipBlurPSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    flipBlurPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipBlurPSO.SetVertexShader(g_pFlipBackgroundVS, sizeof(g_pFlipBackgroundVS));
    flipBlurPSO.SetPixelShader(g_pFlipSmoothPS, sizeof(g_pFlipSmoothPS));
    flipBlurPSO.SetSampleMask(UINT_MAX);
    flipBlurPSO.SetRenderTargetFormat(SSF_THICK, DXGI_FORMAT_UNKNOWN, 1, 0);
    flipBlurPSO.Finalize(device);
    m_PSOs["flipBlurPSO"] = flipBlurPSO;
    psoNames.push_back("flipBlurPSO");

    // composite: reconstruct + refract + Beer-Lambert + Fresnel -> backbuffer
    flipCompositePSO.SetInputLayout(0, nullptr);
    flipCompositePSO.SetRootSignature(g_FLIP_RS);
    flipCompositePSO.SetRasterizerState(noneCullRasterizer);
    flipCompositePSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    flipCompositePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    flipCompositePSO.SetVertexShader(g_pFlipBackgroundVS, sizeof(g_pFlipBackgroundVS));
    flipCompositePSO.SetPixelShader(g_pFlipCompositePS, sizeof(g_pFlipCompositePS));
    flipCompositePSO.SetSampleMask(UINT_MAX);
    flipCompositePSO.SetRenderTargetFormat(backBufferFormat, DXGI_FORMAT_UNKNOWN, 1, 0);
    flipCompositePSO.Finalize(device);
    m_PSOs["flipCompositePSO"] = flipCompositePSO;
    psoNames.push_back("flipCompositePSO");

    // ---- Compute PSOs (data-driven table) ----
    // 새 compute 셰이더 추가: CompiledShaders 헤더 #include 후 아래 표에 CPSO(...) 한 줄만 추가.
    struct ComputePSODef
    {
        const char* name;
        RootSignature* rs;
        const void* code;
        size_t size;
    };
#define CPSO(psoName, rootSig, blob) {psoName, &rootSig, blob, sizeof(blob)}

    const ComputePSODef computePSODefs[] = {
        CPSO("defaultCPSO", g_U1_RS, g_pDefaultCS),
        CPSO("particleSimulationCPSO", g_U1_C1_RS, g_pParticleSimulationCS),
        CPSO("computeDensityCPSO", g_UUUUSSC_RS, g_pComputeDensityCS),
        CPSO("computePressureCPSO", g_UUUUSSC_RS, g_pComputePressureCS),
        CPSO("computeForcesCPSO", g_UUUUSSC_RS, g_pComputeForcesCS),
        CPSO("sphSimulationCPSO", g_UUUUSSC_RS, g_pSPHSimulationCS),
        // sph counting sort
        CPSO("pass0CPSO", g_UUC_RS, g_pClearCounterCS),
        CPSO("pass1CPSO", g_UUUC_RS, g_pCountingCS),
        CPSO("pass2aCPSO", g_SUUC_RS, g_pScanBlocksCS),
        CPSO("pass2bCPSO", g_USC_RS, g_pAddBlockOffsetsCS),
        CPSO("pass3CPSO", g_UUUUSSC_RS, g_pScatterCS),
        // stable fluids
        CPSO("sourcingCPSO", g_U2_C2_RS, g_pSourcingCS),
        CPSO("computeCurlCPSO", g_S1_U1_C1_RS, g_pComputeCurlCS),
        CPSO("vorticityConfinementCPSO", g_S1_U1_C1_RS, g_pVorticityConfinementCS),
        CPSO("advectionCPSO", g_S2_U2_C1_RS, g_pAdvectionCS),
        CPSO("computeDivergenceCPSO", g_S1_U3_C1_RS, g_pComputeDivergenceCS),
        CPSO("jacobiCPSO", g_S2_U1_C1_RS, g_pJacobiCS),
        CPSO("computeFinalVelocityCPSO", g_S1_U1_C1_RS, g_pComputeFinalVelocityCS),
        // noise
        CPSO("perlinNoiseCPSO", g_U1_RS, g_pPerlinNoiseCS),
        CPSO("curlNoiseCPSO", g_S1_U1_RS, g_pCurlNoiseCS),
        CPSO("curlSimulationCPSO", g_CurlNoiseSimulation_RS, g_pCurlNoiseSimulationCS),
        CPSO("noiseSourcingCPSO", g_NoiseSourcing_RS, g_pNoiseSourcingCS),

        // flip water : {UAV u0 = particles, CBV b0 = FlipConstant}
        CPSO("flipBallisticCPSO", g_UC_RS, g_pFlipBallisticCS),
    };
#undef CPSO

    for (const ComputePSODef& def : computePSODefs)
    {
        ComputePSO pso;
        pso.SetRootSignature(*def.rs);
        pso.SetComputeShader(def.code, def.size);
        pso.Finalize(device);
        m_CPSOs[def.name] = pso;
        cpsoNames.push_back(def.name);
    }
}

ID3D12PipelineState* Renderer::GetPSO(std::string psoName)
{
    return m_PSOs[psoName].GetPSO();
}
