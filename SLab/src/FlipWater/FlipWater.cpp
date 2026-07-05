#include "FlipWater.h"

#include "GraphicsCommon.h" 
#include "Engine/World.h"   
#include "Renderer.h"       
#include "RootSignature.h"

using namespace Graphics;
using namespace std;

FlipWater::FlipWater()
{
}

void FlipWater::Initialize(ID3D12Device5* device, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
    m_cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    InitCommands();
    InitResources(cmdAlloc, cmdList);
}

void FlipWater::InitCommands()
{
    if (m_world && m_world->GetDevice())
    {
        auto device = m_world->GetDevice();
        ThrowIfFailed(
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr,
                                                IID_PPV_ARGS(&m_commandList)));
        m_commandList->Close();
    }
}

void FlipWater::InitResources(ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
    // EXACT dam seed from the FlipWater tutorial (1 m^3 tank [0,1]^3, 64^3 grid):
    // fill x < damX, y < damY, full z, 2x2x2 jittered particles per cell. Same
    // deterministic hash -> identical particle count (768,800) and positions.
    auto hash01 = [](unsigned int x) -> float {
        x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
        return (float)(x & 0xffffff) / 16777216.0f;
    };
    const int NX = 64, NY = 64, NZ = 64;
    const float h = 1.0f / 64.0f;
    const float damX = 0.5f, damY = 0.8f;

    m_particles.clear();
    unsigned int id = 1;
    for (int k = 1; k < NZ - 1; ++k)
        for (int j = 1; j < NY - 1; ++j)
            for (int i = 1; i < NX - 1; ++i)
            {
                if ((i + 1) * h > damX || (j + 1) * h > damY) continue;
                for (int s = 0; s < 8; ++s)
                {
                    float jx = (hash01(id * 3 + 0) - 0.5f) * 0.25f;
                    float jy = (hash01(id * 3 + 1) - 0.5f) * 0.25f;
                    float jz = (hash01(id * 3 + 2) - 0.5f) * 0.25f;
                    ++id;
                    FlipParticle p{};
                    p.position = Vector3((i + ((s >> 0 & 1) + 0.5f) * 0.5f + jx) * h,
                                         (j + ((s >> 1 & 1) + 0.5f) * 0.5f + jy) * h,
                                         (k + ((s >> 2 & 1) + 0.5f) * 0.5f + jz) * h);
                    p.velocity = Vector3(0.f, 0.f, 0.f);
                    p.radius = 0.006f;
                    p.color = Vector3(0.22f, 0.46f, 0.88f); // tutorial water-blue
                    m_particles.push_back(p);
                }
            }
    m_particleCount = (int)m_particles.size();

    // GPU particle buffer (UAV) + staging upload. The copy is recorded onto the
    // EXTERNAL resource cmd list (flushed once by RenderEngine::InitScene).
    Graphics::utility->CreateBuffer(m_particles, m_particleBuffer, m_particleUpload,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, cmdList);
    // Render passes bind the particle buffer as a root SRV (RenderEngine::RenderFluid),
    // so no descriptor-heap SRV is needed here.

    // Sim constants.
    FlipConstant c = {};
    c.particleCount = (UINT)m_particleCount;
    c.dt = 1.0f / 120.0f;
    c.gravity = Vector3(0.f, -9.81f, 0.f);
    c.boundsMin = Vector3(0.016f, 0.016f, 0.016f); // 1-cell solid walls of the [0,1]^3 tank
    c.boundsMax = Vector3(0.984f, 0.984f, 0.984f);
    m_cb.Initialize(c);

    // Per-frame SSF render constant (filled by RenderEngine each frame).
    FlipFrameConstant fc = {};
    m_frameCB.Initialize(fc);
}

void FlipWater::Tick(float deltaTime)
{
    float dt = (deltaTime < 1.0f / 60.0f) ? deltaTime : 1.0f / 60.0f; // clamp big load hitches
    m_cb.localConstant.dt = dt;
    m_cb.localConstant.particleCount = (UINT)m_particleCount;
    m_cb.Update();

    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    // GUI Reset: re-copy the initial seed (still held in the upload buffer) back into
    // the particle buffer, then skip gravity this frame so particles are exactly initial.
    if (m_resetRequested)
    {
        m_resetRequested = false;
        UINT64 bytes = (UINT64)m_particleCount * sizeof(FlipParticle);
        auto toCopy = CD3DX12_RESOURCE_BARRIER::Transition(m_particleBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                           D3D12_RESOURCE_STATE_COPY_DEST);
        m_commandList->ResourceBarrier(1, &toCopy);
        m_commandList->CopyBufferRegion(m_particleBuffer.Get(), 0, m_particleUpload.Get(), 0, bytes);
        auto toCommon = CD3DX12_RESOURCE_BARRIER::Transition(m_particleBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                             D3D12_RESOURCE_STATE_COMMON);
        m_commandList->ResourceBarrier(1, &toCommon);
        return;
    }

    // One compute pass: gravity + integrate + wall clamp.
    SetCPSO("flipBallisticCPSO");
    m_commandList->SetComputeRootUnorderedAccessView(0, m_particleBuffer->GetGPUVirtualAddress()); // u0
    m_commandList->SetComputeRootConstantBufferView(1, m_cb.GetGPUAddress());                      // b0

    UINT groups = (m_particleCount + FLIP_GROUP_SIZE - 1) / FLIP_GROUP_SIZE;
    m_commandList->Dispatch(groups, 1, 1);
}

void FlipWater::Execute(ID3D12CommandQueue* commandQueue)
{
    m_commandList->Close();
    ID3D12CommandList* commands[] = {m_commandList.Get()};
    commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);
}

void FlipWater::SetCPSO(const std::string& psoName)
{
    ComputePSO cpso = Renderer::GetComputePSO(psoName);
    m_commandList->SetPipelineState(cpso.GetPSO());
    m_commandList->SetComputeRootSignature(cpso.GetRootSignature()->GetSignature());
}
