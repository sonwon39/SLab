#pragma once

#include <vector>
#include <string>
#include "directxtk12\SimpleMath.h"
#include "wrl.h"
#include "d3d12.h"
#include "PipelineState.h"
#include "Core/ConstantBuffer.h"
#include "FlipParticle.h"

class RenderEngine;

// FLIP water sim module (build-along). STEP 1 = gravity-only ballistic particles
// rendered as points. Mirrors the SPH module lifecycle:
//   Initialize : creates the compute cmd list + uploads the seed on the external
//                resource cmd list.
//   Tick       : records the compute passes into its own m_commandList.
//   Execute    : close + submit m_commandList to the shared queue.
//   Render     : point draw onto the main back-buffer cmd list.
class FlipWater
{
    friend class RenderEngine;

  public:
    FlipWater();

    void Initialize(ID3D12Device5* device, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);
    void InitCommands();
    void InitResources(ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

  public:
    void Tick(float deltaTime);
    void Execute(ID3D12CommandQueue* commandQueue);

    // per-frame SSF render constant (camera basis, projA, radius, ...).
    // The render passes themselves live in RenderEngine::RenderFluid.
    void SetFrame(const FlipFrameConstant& fc)
    {
        m_frameCB.localConstant = fc;
        m_frameCB.Update();
    }
    int ParticleCount() const { return m_particleCount; }
    D3D12_GPU_VIRTUAL_ADDRESS ParticleVA() const { return m_particleBuffer->GetGPUVirtualAddress(); }
    D3D12_GPU_VIRTUAL_ADDRESS FrameCBVA() const { return m_frameCB.GetGPUAddress(); }

    // GUI reset: restore particles to their initial seed (applied on the next Tick).
    void Reset()
    {
        m_resetRequested = true;
    }

  private:
    void SetCPSO(const std::string& psoName);

  private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

    int m_particleCount = 0;
    std::vector<FlipParticle> m_particles;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_particleBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_particleUpload; // kept alive until the upload executes

    ConstantBuffer<FlipConstant> m_cb;
    ConstantBuffer<FlipFrameConstant> m_frameCB;
    UINT m_cbvSrvDescriptorSize = 0;
    bool m_resetRequested = false;
};
