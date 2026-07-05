#include "SPH.h"
#include <random>
#include <iostream>

#include "GraphicsCommon.h"
#include "Engine/World.h"
#include "Renderer.h"
#include "RootSignature.h"

using namespace Graphics;
using namespace Renderer;

SPH::SPH()
{
}

void SPH::Initialize(ID3D12Device5* device, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
    using namespace Graphics;

    m_cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_sphParticleConstant.particleCount = sphCurrParticleCount;
    // particleRadius 하나로부터 시뮬레이션 상수 자동 도출 (mass=1, 2D cubic spline)
    //   dx_eq = 2·radius      평형 packing 시 입자 중심 간 간격 (= 직경)
    //   h     = 1.3·dx_eq     cubic spline 2D 권장 — support 안에 이웃 ~21개
    //   rho0  = 1/dx_eq²      연속체 한계 ρ ≈ m·n = m/dx² (m=1)
    particleSpacing = 2.0f * particleRadius;
    h = particleSpacing;
    rho0 = 1.0f / (particleSpacing * particleSpacing * particleSpacing);

    m_sphParticleConstant.h = h;
    m_sphParticleConstant.hd = 1.0f / (h * h * h);
    m_sphParticleConstant.kernelCoefficient = kernelCoeff;
    m_sphParticleConstant.rho0 = rho0;
    m_sphParticleConstant.k = k;
    m_sphParticleConstant.mu = mu;

    m_sphParticleConstant.gGridMin = Vector3(-0.75f, -0.9f, -0.75f);
    m_sphParticleConstant.gGridMax = Vector3(0.75f, 0.9f, 0.75f);
    cellSize = 2 * h;
    m_sphParticleConstant.gCellSize = cellSize;

    Vector3 gridLen = m_sphParticleConstant.gGridMax - m_sphParticleConstant.gGridMin;
    gridDim = Vector3(gridLen.x / (cellSize), gridLen.y / (cellSize), gridLen.z / (cellSize));
    cellCount = int(gridDim.x * gridDim.y * gridDim.z);

    m_sphParticleConstant.gGridDim = gridDim;
    m_sphParticleConstant.gCellCount = cellCount;
    m_sphParticleConstant.gScanCount = cellCount;

    // Pass2 재귀 scan 레벨 산정.
    // 매 level 의 blockCount(=ceil(scanCount/GROUP_SIZE)) 가 다음 level 의 scanCount 가 됨.
    // blockCount<=1 인 level 은 단일 블록 안에서 끝나는 terminal — 더 이상 재귀 안 함.
    m_pass2Levels.clear();
    {
        UINT sc = (UINT)cellCount;
        while (true)
        {
            Pass2Level lvl;
            lvl.scanCount = sc;
            lvl.blockCount = (sc + GROUP_SIZE - 1) / GROUP_SIZE;
            m_pass2Levels.push_back(std::move(lvl));
            if (m_pass2Levels.back().blockCount <= 1)
                break;
            sc = m_pass2Levels.back().blockCount;
        }
    }

    // per-frame 시뮬레이션 전용 command list/allocator 생성 (StableFluids 와 동일 패턴).
    InitCommands();

    // heap 생성
    utility->CreateDescriptorHeap(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_sphParticleUAVHeap[0], 0,
                                  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    utility->CreateDescriptorHeap(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_sphParticleUAVHeap[1], 0,
                                  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    utility->CreateDescriptorHeap(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_sphParticleSRVHeap, 0,
                                  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    utility->CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_sortedSphParticleHeap, 0,
                                  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    InitParticleCPU();
    InitParticleGPU(cmdAlloc, cmdList);

    utility->CreateConstantBuffer(sizeof(SPHParticleLocalConstant), m_sphParticleLocalCB, &pSPHParticleLocalCB);
    memcpy(pSPHParticleLocalCB, &m_sphParticleConstant, sizeof(SPHParticleLocalConstant));

    // Pass2 레벨별 CB. gScanCount 만 다르고 나머지는 m_sphParticleConstant 와 동일.
    // Pass2 셰이더는 gScanCount 만 읽으므로 Tick 의 particleCount/dt 갱신과 무관.
    for (auto& lvl : m_pass2Levels)
    {
        utility->CreateConstantBuffer(sizeof(SPHParticleLocalConstant), lvl.cb, &lvl.pCb);
        SPHParticleLocalConstant scanCB = m_sphParticleConstant;
        scanCB.gScanCount = lvl.scanCount;
        memcpy(lvl.pCb, &scanCB, sizeof(SPHParticleLocalConstant));
    }
}

void SPH::InitCommands()
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

void SPH::UpdateConstants(float deltaTime)
{
    if (countTick < sphMaxParticleCount)
    {
        countTick += sphCountIncreaseSpeed * deltaTime;

        if (countTick >= sphMaxParticleCount)
            countTick = float(sphMaxParticleCount);

        sphCurrParticleCount = int(countTick);
    }

    m_sphParticleConstant.particleCount = sphCurrParticleCount;
    m_sphParticleConstant.dt = 1 / 300.f;
    memcpy(pSPHParticleLocalCB, &m_sphParticleConstant, sizeof(SPHParticleLocalConstant));
}

// 한 프레임의 시뮬레이션 전체 (counting-sort + density/pressure/forces/simulation)
// 를 m_commandList 한 개에 기록.
//   진입 상태 가정: SPH 의 모든 GPU buffer 가 COMMON (이전 ExecuteCommandLists 이후 decay).
//   종료 상태: m_commandList 가 열린(open) 채로 모든 dispatch 기록 완료. Execute() 가 close+submit 담당.
void SPH::Tick(float deltaTime)
{
    UpdateConstants(deltaTime);

    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    // ── Counting-sort ───────────────────────────────────────────────────────
    // 종료 후 상태:
    //   m_cellCounterBuffer, m_particleCellIdBuffer, m_pass2Levels[0].output → NON_PIXEL_SHADER_RESOURCE
    //   m_scatterCounterBuffer, m_sortedSphParticleBuffer, m_sortedIndicesBuffer, m_sphParticleBuffer[idx] → UAV
    Sort();

    // ── Sort → Compute Density ──────────────────────────────────────────────
    // Sort:Pass3 가 방금 쓴 m_sortedSphParticleBuffer / m_sortedIndicesBuffer 를 density 가 UAV 로 다시 읽음.
    // 동일 UAV write→read 이므로 UAV barrier 필요.
    {
        D3D12_RESOURCE_BARRIER bs[2] = {
            CD3DX12_RESOURCE_BARRIER::UAV(m_sortedSphParticleBuffer.Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(m_sortedIndicesBuffer.Get()),
        };
        m_commandList->ResourceBarrier(2, bs);
    }

    // ── Density → Pressure ──────────────────────────────────────────────────
    // density 가 curr_particles.density 를 쓰고 pressure 가 이를 읽는다 (동일 UAV).
    Compute("computeDensityCPSO");
    {
        auto b = CD3DX12_RESOURCE_BARRIER::UAV(m_sphParticleBuffer[1 - m_sphHeapIdx].Get());
        m_commandList->ResourceBarrier(1, &b);
    }

    // ── Pressure → Forces ───────────────────────────────────────────────────
    // pressure 가 curr_particles.pressure 를 쓰고 forces 가 density/pressure 를 모두 읽는다.
    Compute("computePressureCPSO");
    {
        auto b = CD3DX12_RESOURCE_BARRIER::UAV(m_sphParticleBuffer[1 - m_sphHeapIdx].Get());
        m_commandList->ResourceBarrier(1, &b);
    }

    // ── Forces → Simulation ─────────────────────────────────────────────────
    // forces 가 curr_particles.acceleration 을 쓰고 simulation 이 이를 읽는다.
    Compute("computeForcesCPSO");
    {
        auto b = CD3DX12_RESOURCE_BARRIER::UAV(m_sphParticleBuffer[1 - m_sphHeapIdx].Get());
        m_commandList->ResourceBarrier(1, &b);
    }

    Compute("sphSimulationCPSO");
}

void SPH::Execute(ID3D12CommandQueue* commandQueue)
{
    m_commandList->Close();
    ID3D12CommandList* commands[] = {m_commandList.Get()};
    commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);
}

void SPH::SetCPSO(const std::string& psoName)
{
    ComputePSO cpso = Renderer::GetComputePSO(psoName);
    m_commandList->SetPipelineState(cpso.GetPSO());
    m_commandList->SetComputeRootSignature(cpso.GetRootSignature()->GetSignature());
}

// 버퍼 clear용
void SPH::Pass0()
{
    SetCPSO("pass0CPSO");

    m_commandList->SetComputeRootUnorderedAccessView(0, m_cellCounterBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(1, m_scatterCounterBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootConstantBufferView(2, m_sphParticleLocalCB->GetGPUVirtualAddress());

    UINT groupCount = (cellCount + GROUP_SIZE - 1) / GROUP_SIZE;
    m_commandList->Dispatch(groupCount, 1, 1);
}

void SPH::Pass1()
{
    auto& particle = m_sphParticleBuffer[m_sphHeapIdx];

    SetCPSO("pass1CPSO");

    m_commandList->SetComputeRootUnorderedAccessView(0, particle->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(1, m_cellCounterBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(2, m_particleCellIdBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootConstantBufferView(3, m_sphParticleLocalCB->GetGPUVirtualAddress());

    UINT groupXCount = (sphMaxParticleCount + GROUP_SIZE - 1) / GROUP_SIZE;
    m_commandList->Dispatch(groupXCount, 1, 1);
}

// Pass2a: per-block exclusive scan + emit block totals.
//   bindings (ScanBlocksCS.hlsl): t0=gInput(SRV), u0=gOutput(UAV), u1=gBlockSums(UAV), b0=GridCB(CBV)
//   input:  level 0 → m_cellCounterBuffer,  level k>0 → m_pass2Levels[k-1].blockSums
//   output, blockSums, cb 는 모두 m_pass2Levels[level] 안에 있음.
void SPH::Pass2a(int level)
{
    auto& lvl = m_pass2Levels[level];
    D3D12_GPU_VIRTUAL_ADDRESS in_va = (level == 0) ? m_cellCounterBuffer->GetGPUVirtualAddress()
                                                   : m_pass2Levels[level - 1].blockSums->GetGPUVirtualAddress();

    m_commandList->SetComputeRootShaderResourceView(0, in_va);
    m_commandList->SetComputeRootUnorderedAccessView(1, lvl.output->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(2, lvl.blockSums->GetGPUVirtualAddress());
    m_commandList->SetComputeRootConstantBufferView(3, lvl.cb->GetGPUVirtualAddress());
    m_commandList->Dispatch(lvl.blockCount, 1, 1);
}

// Pass2b: m_pass2Levels[level].output[i] += m_pass2Levels[level+1].output[i / GROUP_SIZE]
//   bindings (AddBlockOffsetsCS.hlsl): u0=gPartial(UAV), t0=gScannedBlockSums(SRV), b0=GridCB(CBV)
//   terminal level (lastLevel) 에서는 호출하지 않음 — 재귀 종료.
void SPH::Pass2b(int level)
{
    auto& lvl = m_pass2Levels[level];
    auto& sub = m_pass2Levels[level + 1]; // 한 level 깊은 곳의 output = scanned block sums

    m_commandList->SetComputeRootUnorderedAccessView(0, lvl.output->GetGPUVirtualAddress());
    m_commandList->SetComputeRootShaderResourceView(1, sub.output->GetGPUVirtualAddress());
    m_commandList->SetComputeRootConstantBufferView(2, lvl.cb->GetGPUVirtualAddress());
    m_commandList->Dispatch(lvl.blockCount, 1, 1);
}

// 재귀 전역 exclusive scan.
//   scan(level):
//     Pass2a(level)
//     if terminal: return
//     transition blockSums[level] UAV→SRV     (다음 level 의 input)
//     scan(level + 1)                          ← 재귀
//     transition output[level+1] UAV→SRV      (Pass2b 의 gScannedBlockSums)
//     Pass2b(level)
//     transition들 다시 UAV 로 복원            (다음 프레임 호출 위해)
//
// 진입 조건: m_cellCounterBuffer 가 NON_PIXEL_SHADER_RESOURCE 상태.
//           m_pass2Levels 모든 buffer 들이 UNORDERED_ACCESS 상태.
// 종료 조건: 모든 buffer 상태가 진입 때와 동일하게 복원됨 (per-frame 재실행 가능).
void SPH::Pass2(int level)
{
    auto& lvl = m_pass2Levels[level];
    const int lastLevel = (int)m_pass2Levels.size() - 1;

    // ── Pass2a dispatch ──────────────────────────────────────────────────
    SetCPSO("pass2aCPSO");
    Pass2a(level);

    // terminal level: blockCount==1 → 단일 블록 안에서 scan 완료. 더 재귀 안 함.
    if (level == lastLevel)
        return;

    // 다음 level 이 blockSums 를 SRV 로 읽어야 함. Transition 자체가 UAV write 완료까지 implicit sync.
    {
        auto b = CD3DX12_RESOURCE_BARRIER::Transition(lvl.blockSums.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_commandList->ResourceBarrier(1, &b);
    }

    // ── 재귀 ─────────────────────────────────────────────────────────────
    Pass2(level + 1);

    // 재귀에서 돌아온 시점: m_pass2Levels[level+1].output 가 전역 scan 완료 상태 (UAV).
    // Pass2b 는 이걸 SRV 로 읽음.
    {
        auto b = CD3DX12_RESOURCE_BARRIER::Transition(m_pass2Levels[level + 1].output.Get(),
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_commandList->ResourceBarrier(1, &b);
    }

    // ── Pass2b dispatch ──────────────────────────────────────────────────
    SetCPSO("pass2bCPSO");
    Pass2b(level);

    // ── 상태 복원 (다음 프레임 호출에서 UAV 로 다시 쓸 수 있도록) ─────────
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(lvl.blockSums.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_pass2Levels[level + 1].output.Get(),
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        m_commandList->ResourceBarrier(2, barriers);
    }
}

// Pass3: scatter — particle 들을 cellId 기준 정렬된 슬롯에 복사.
//   bindings (ScatterCS.hlsl):
//     u0 = gParticles       (RW, source)
//     u1 = gScatterCounter  (RW, per-cell write cursor, 호출 전 0 으로 클리어 필수)
//     u2 = gSortedParticles (RW, destination)
//     t0 = gParticleCellId  (SRV, from Pass1)
//     t1 = gCellStart       (SRV, = m_pass2Levels[0].output, from Pass2)
//     b0 = gCB              (CBV, gCB.particleCount 으로 early-exit)
//   Dispatch ceil(sphMaxParticleCount / GROUP_SIZE) — 셰이더가 particleCount 로 early-exit.
void SPH::Pass3()
{
    auto& particle = m_sphParticleBuffer[m_sphHeapIdx];
    SetCPSO("pass3CPSO");

    m_commandList->SetComputeRootUnorderedAccessView(0, particle->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(1, m_scatterCounterBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(2, m_sortedSphParticleBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(3, m_sortedIndicesBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootShaderResourceView(4, m_particleCellIdBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootShaderResourceView(5, m_pass2Levels[0].output->GetGPUVirtualAddress());
    m_commandList->SetComputeRootConstantBufferView(6, m_sphParticleLocalCB->GetGPUVirtualAddress());

    UINT groupXCount = (sphMaxParticleCount + GROUP_SIZE - 1) / GROUP_SIZE;
    m_commandList->Dispatch(groupXCount, 1, 1);
}

// 한 프레임의 counting-sort 파이프라인 전체 (Pass0 → 1 → 2 → 3) 를
// 올바른 GPU sync barrier 와 함께 m_commandList 안에서 실행.
//
// 진입 상태 가정: 관련 buffer 들이 COMMON (이전 ExecuteCommandLists 후 decay).
//   → 첫 SetCompute*ResourceView 호출이 implicit promotion (COMMON → UAV/SRV).
// 종료 상태 (decay 직전):
//   m_cellCounterBuffer, m_particleCellIdBuffer, m_pass2Levels[0].output → NON_PIXEL_SHADER_RESOURCE
//   m_scatterCounterBuffer, m_sortedSphParticleBuffer, m_sortedIndicesBuffer → UAV
//   Pass2 내부 level buffer 들 → UAV (Pass2 자체 invariant)
// 다음 ExecuteCommandLists 후 모두 COMMON 으로 decay → 다음 프레임 재진입 가능.
void SPH::Sort()
{
    Pass0();

    // Pass0 → Pass1: cellCounter 의 0-clear 가 Pass1 의 InterlockedAdd 보다 먼저 끝나야 함.
    {
        D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::UAV(m_cellCounterBuffer.Get());
        m_commandList->ResourceBarrier(1, &b);
    }

    Pass1();

    // Pass1 → Pass2: Pass2a 의 level-0 입력이 m_cellCounterBuffer (SRV 슬롯).
    //   Transition 은 prior UAV write 도 자동 drain 시켜주므로 별도 UAV barrier 불필요.
    //   m_particleCellIdBuffer 는 Pass2 가 안 건드리므로 Pass3 직전에 한 번에 transition.
    {
        D3D12_RESOURCE_BARRIER b =
            CD3DX12_RESOURCE_BARRIER::Transition(m_cellCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_commandList->ResourceBarrier(1, &b);
    }

    Pass2();

    // Pass2 → Pass3:
    //   Pass3 의 SRV 입력 둘 (cellStart = m_pass2Levels[0].output, particleCellId) 를 UAV→SRV.
    //   m_scatterCounterBuffer 는 Pass0 가 clear, Pass3 가 InterlockedAdd — 사이 UAV barrier 필요.
    {
        D3D12_RESOURCE_BARRIER bs[3] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_pass2Levels[0].output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_particleCellIdBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::UAV(m_scatterCounterBuffer.Get()),
        };
        m_commandList->ResourceBarrier(3, bs);
    }

    Pass3();
}

void SPH::Compute(const std::string& psoName)
{
    SetCPSO(psoName);

    auto& prevParticles = m_sphParticleBuffer[m_sphHeapIdx];
    auto& currParticles = m_sphParticleBuffer[1 - m_sphHeapIdx];

    m_commandList->SetComputeRootUnorderedAccessView(0, prevParticles->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(1, currParticles->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(2, m_sortedSphParticleBuffer->GetGPUVirtualAddress());
    m_commandList->SetComputeRootUnorderedAccessView(3, m_sortedIndicesBuffer->GetGPUVirtualAddress());

    m_commandList->SetComputeRootShaderResourceView(4, m_pass2Levels[0].output->GetGPUVirtualAddress());
    m_commandList->SetComputeRootShaderResourceView(5, m_cellCounterBuffer->GetGPUVirtualAddress());

    m_commandList->SetComputeRootConstantBufferView(6, m_sphParticleLocalCB->GetGPUVirtualAddress());

    UINT threadXCount = (sphCurrParticleCount + GROUP_SIZE - 1) / GROUP_SIZE;
    m_commandList->Dispatch(threadXCount, 1, 1);
}

void SPH::Render(ID3D12GraphicsCommandList* commandList)
{
    m_sphHeapIdx = (m_sphHeapIdx + 1) % 2;

    ID3D12DescriptorHeap* heaps[] = {m_sphParticleSRVHeap.Get()};

    CD3DX12_GPU_DESCRIPTOR_HANDLE sphHandle(m_sphParticleSRVHeap->GetGPUDescriptorHandleForHeapStart(), m_sphHeapIdx,
                                            m_cbvSrvDescriptorSize);

    commandList->SetDescriptorHeaps(1, heaps);
    commandList->SetGraphicsRootDescriptorTable(0, sphHandle);
    commandList->SetGraphicsRootConstantBufferView(2, m_sphParticleLocalCB->GetGPUVirtualAddress());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    commandList->DrawInstanced(sphCurrParticleCount, 1, 0, 0);
}

void SPH::Reset(ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
    InitParticleCPU();
    InitParticleGPU(cmdAlloc, cmdList);

    memcpy(pSPHParticleLocalCB, &m_sphParticleConstant, sizeof(SPHParticleLocalConstant));
}

void SPH::InitParticleCPU()
{
    sphCurrParticleCount = 0;
    countTick = 0.f;

    std::random_device rd;
    std::mt19937 gen(rd());

    m_sphParticles.resize(sphMaxParticleCount);

    Vector3 basePosition0(-0.7f, 0.4f, 0.f);
    Vector3 basePosition1(0.7f, 0.4f, 0.f);
    Vector3 baseVelocity0(1.f, 0.f, 0.f);
    Vector3 baseVelocity1(-1.f, 0.f, 0.f);

    float PI = 3.141592f;
    std::uniform_real_distribution<float> randomTheta(-PI, PI);
    std::uniform_real_distribution<float> randomDist(0.f, 0.2f);
    std::uniform_real_distribution<float> velDist(1.f, 2.f);

    // structured buffer cpu 데이터 초기화
    for (size_t i = 0; i < sphMaxParticleCount; i++)
    {
        SPHParticle& p = m_sphParticles[i];
        float theta = randomTheta(gen);
        if (i % 2 == 0)
        {
            p.position = basePosition0 + Vector3(0.f, std::cos(theta), -std::sin(theta)) * randomDist(gen);
            p.velocity = baseVelocity0 * velDist(gen);
        }
        else
        {
            p.position = basePosition1 + Vector3(0.f, std::cos(theta), -std::sin(theta)) * randomDist(gen);
            p.velocity = baseVelocity1 * velDist(gen);
        }

        p.color = Vector3(0.0f, 0.0f, 1.0f);
        p.acceleration = Vector3(0.f, 0.f, 0.f);
        p.radius = particleRadius;
    }

    m_cellCounter.resize(cellCount, 0);
    m_particleCellId.resize(sphMaxParticleCount, 0);
    m_scatterCounter.resize(cellCount, 0);
}

void SPH::InitParticleGPU(ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
    using namespace Graphics;

    // structured gpu buffer 생성
    D3D12_RESOURCE_FLAGS uavFlag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    for (size_t i = 0; i < 2; i++)
    {
        Graphics::utility->CreateBuffer(m_sphParticles, m_sphParticleBuffer[i], m_sphParticleUpload[i], uavFlag,
                                        cmdList);
    }
    Graphics::utility->CreateBuffer(m_cellCounter, m_cellCounterBuffer, m_cellCounterUpload, uavFlag, cmdList);
    Graphics::utility->CreateBuffer(m_particleCellId, m_sortedIndicesBuffer, m_sortedIndicesUpload, uavFlag, cmdList);
    Graphics::utility->CreateBuffer(m_particleCellId, m_particleCellIdBuffer, m_particleCellIdUpload, uavFlag, cmdList);
    Graphics::utility->CreateBuffer(m_sphParticles, m_sortedSphParticleBuffer, m_sortedSphParticleUpload, uavFlag,
                                    cmdList);
    Graphics::utility->CreateBuffer(m_scatterCounter, m_scatterCounterBuffer, m_scatterCounterUpload, uavFlag, cmdList);

    // Pass2 각 level의 output[scanCount] + blockSums[blockCount] 버퍼를 0으로 초기화 생성.
    // 실제 값은 Pass2a/Pass2b dispatch 가 채움.
    for (auto& lvl : m_pass2Levels)
    {
        std::vector<UINT> outInit(lvl.scanCount, 0);
        std::vector<UINT> bsInit(lvl.blockCount, 0);
        Graphics::utility->CreateBuffer(outInit, lvl.output, lvl.outputUpload, uavFlag, cmdList);
        Graphics::utility->CreateBuffer(bsInit, lvl.blockSums, lvl.blockSumsUpload, uavFlag, cmdList);
    }

    // view 생성
    CD3DX12_CPU_DESCRIPTOR_HANDLE prev_handle(m_sphParticleUAVHeap[0]->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE curr_handle(m_sphParticleUAVHeap[1]->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle(m_sphParticleSRVHeap->GetCPUDescriptorHandleForHeapStart());
    utility->CreateStructuredResourceView(m_sphParticleBuffer[0], DXGI_FORMAT_UNKNOWN, prev_handle, DescriptorType::UAV,
                                          sphMaxParticleCount, sizeof(SPHParticle));
    utility->CreateStructuredResourceView(m_sphParticleBuffer[1], DXGI_FORMAT_UNKNOWN, curr_handle, DescriptorType::UAV,
                                          sphMaxParticleCount, sizeof(SPHParticle));
    utility->CreateStructuredResourceView(m_sphParticleBuffer[0], DXGI_FORMAT_UNKNOWN, srv_handle, DescriptorType::SRV,
                                          sphMaxParticleCount, sizeof(SPHParticle));

    prev_handle.Offset(1, m_cbvSrvDescriptorSize);
    curr_handle.Offset(1, m_cbvSrvDescriptorSize);
    srv_handle.Offset(1, m_cbvSrvDescriptorSize);

    utility->CreateStructuredResourceView(m_sphParticleBuffer[1], DXGI_FORMAT_UNKNOWN, prev_handle, DescriptorType::UAV,
                                          sphMaxParticleCount, sizeof(SPHParticle));
    utility->CreateStructuredResourceView(m_sphParticleBuffer[0], DXGI_FORMAT_UNKNOWN, curr_handle, DescriptorType::UAV,
                                          sphMaxParticleCount, sizeof(SPHParticle));
    utility->CreateStructuredResourceView(m_sphParticleBuffer[1], DXGI_FORMAT_UNKNOWN, srv_handle, DescriptorType::SRV,
                                          sphMaxParticleCount, sizeof(SPHParticle));

    // 정렬용 view 생성
    utility->CreateStructuredResourceView(m_sortedSphParticleBuffer, DXGI_FORMAT_UNKNOWN,
                                          m_sortedSphParticleHeap->GetCPUDescriptorHandleForHeapStart(),
                                          DescriptorType::SRV, sphMaxParticleCount, sizeof(SPHParticle));
}
