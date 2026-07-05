#pragma once

#include <vector>
#include "directxtk12\SimpleMath.h"
#include "wrl.h"
#include "d3d12.h"

#include "particle.h"
#include "PipelineState.h"

class SPH
{
  public:
    SPH();

  public:
    // 초기 particle upload 는 외부 resource cmdList 로 처리 (one-shot).
    // 그 외 per-frame 시뮬레이션은 SPH 가 자체 보유한 m_commandList 에 모두 기록한다.
    void Initialize(ID3D12Device5* device, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);
    void InitCommands();
    void InitParticleCPU();
    void InitParticleGPU(ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

  public:
    // per-frame 시뮬레이션 전체를 m_commandList 에 기록 (Sort → Density → Pressure → Forces → Simulation).
    void Tick(float deltaTime);
    // m_commandList Close + 외부 큐로 제출.
    void Execute(ID3D12CommandQueue* commandQueue);

    // render 는 메인 백버퍼 cmdList 가 필요하므로 외부 cmdList 수신 유지.
    void Render(ID3D12GraphicsCommandList* commandList);

    void Reset(ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

  public:
    ID3D12DescriptorHeap* GetParticleUAVHeap(int idx)
    {
        return m_sphParticleUAVHeap[idx].Get();
    }

  private:
    void UpdateConstants(float deltaTime);
    void SetCPSO(const std::string& psoName);

    // counting-sort 파이프라인 — 모두 m_commandList 위에 기록.
    void Sort();
    void Pass0();
    void Pass1();
    // 재귀 전역 exclusive scan. cellCount 가 아무리 커도 동작 (필요한 만큼 level 자동 생성).
    //   Pass2  = orchestrator. PSO/RootSig 전환 + barrier + 재귀 호출 책임.
    //   Pass2a = 한 level의 view 바인딩 + dispatch (Pass1 스타일)
    //   Pass2b = 한 level의 view 바인딩 + dispatch
    // 진입 invariant: m_cellCounterBuffer 는 NON_PIXEL_SHADER_RESOURCE, 모든 m_pass2Levels 버퍼는 UNORDERED_ACCESS.
    // 종료 invariant: 동일 (per-frame 재호출 가능).
    // 최종 결과: m_pass2Levels[0].output (= cellStart, 전역 exclusive scan).
    void Pass2(int level = 0);
    void Pass2a(int level);
    void Pass2b(int level);
    // Pass3: cellId 기준 scatter. 매 frame Pass2 다음에 호출.
    // 호출 전 caller 책임:
    //   1) m_scatterCounterBuffer 를 0 으로 클리어 (ClearUAV 또는 zero-upload + CopyResource)
    //   2) m_particleCellIdBuffer:  UAV → NON_PIXEL_SHADER_RESOURCE
    //   3) m_pass2Levels[0].output: UAV → NON_PIXEL_SHADER_RESOURCE
    // 결과: m_sortedSphParticleBuffer 에 cellId 순으로 정렬된 SPHParticle 들.
    void Pass3();

    // 4 단계 simulation compute — PSO/RootSig set + binding + dispatch 를 일괄 처리.
    //   density / pressure / forces / simulation 모두 동일 binding 레이아웃 (g_UUUUSSC_RS).
    void Compute(const std::string& psoName);

  private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

  private:
    int sphCurrParticleCount = 0;
    float countTick = 0.f;
    float sphCountIncreaseSpeed = 3000.f;
    int sphMaxParticleCount = 30'000;

    std::vector<SPHParticle> m_sphParticles;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_sphParticleBuffer[2];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_sphParticleUpload[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_sphParticleUAVHeap[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_sphParticleSRVHeap;

    SPHParticleLocalConstant m_sphParticleConstant;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_sphParticleLocalCB;
    void* pSPHParticleLocalCB;

    int m_sphHeapIdx = 0;

    // 정렬
  private:
    std::vector<UINT> m_cellCounter;
    std::vector<UINT> m_particleCellId;
    Vector3 gridDim;
    int cellCount;
    float cellSize;

    // Pass1: count
    Microsoft::WRL::ComPtr<ID3D12Resource> m_cellCounterBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_particleCellIdBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_cellCounterUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_particleCellIdUpload;

    // Pass2: 재귀 exclusive scan 의 level 별 자원 묶음.
    //   level 0:  input = m_cellCounterBuffer
    //   level k>0: input = m_pass2Levels[k-1].blockSums
    // 마지막 level은 blockCount==1 (단일 블록 안에서 scan 완료 → terminal, 더 재귀 안 함).
    struct Pass2Level
    {
        UINT scanCount = 0;                            // 이 level 입력 길이
        UINT blockCount = 0;                           // ceil(scanCount / GROUP_SIZE) = 이 level 의 dispatch 그룹 수
        Microsoft::WRL::ComPtr<ID3D12Resource> output; // [scanCount] — exclusive scan 결과
        Microsoft::WRL::ComPtr<ID3D12Resource>
            blockSums;                             // [blockCount] — 각 block 의 inclusive 총합 (다음 level 의 input)
        Microsoft::WRL::ComPtr<ID3D12Resource> cb; // gScanCount = scanCount 로 채운 CB

        // CreateBuffer 가 요구하는 upload counterpart (cmdList 실행까지 살아있어야 함)
        Microsoft::WRL::ComPtr<ID3D12Resource> outputUpload;
        Microsoft::WRL::ComPtr<ID3D12Resource> blockSumsUpload;
        void* pCb = nullptr;
    };
    std::vector<Pass2Level> m_pass2Levels;

    // Pass3: scatter — particle 들을 cellId 순으로 m_sortedSphParticleBuffer 에 복사.
    Microsoft::WRL::ComPtr<ID3D12Resource>
        m_scatterCounterBuffer; // [cellCount] — per-cell write cursor (매 frame 0 으로 클리어)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_scatterCounterUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource>
        m_sortedSphParticleBuffer; // [sphMaxParticleCount] — Pass3 출력 (정렬된 particle)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_sortedSphParticleUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_sortedIndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_sortedIndicesUpload; // [cellCount]
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_sortedSphParticleHeap;

    std::vector<UINT> m_scatterCounter;            // init 용 (zeros)
    std::vector<SPHParticle> m_sortedSphParticles; // init 용

    // sph particle 계수
  public:
    float particleRadius = 0.01f;
    float particleSpacing;
    float h;

    float kernelCoeff = 3.0f / (2.0f * 3.141592f);
    float rho0; // particleRadius로부터 Initialize()에서 자동 계산
    float k = 50000.f;
    float mu = 10.f;

  private:
    UINT m_cbvSrvDescriptorSize = 0;
    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
};
