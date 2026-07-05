// Pass 3: 각 particle 을 cellId 순으로 정렬된 슬롯에 복사한다.
//
// 입력:
//   gParticles[N]        — 정렬 전 particle 배열 (current ping-pong buffer)
//   gParticleCellId[N]   — Pass1 에서 캐시한 각 particle 의 cellId
//   gCellStart[M]        — Pass2 결과 (전역 exclusive scan)  = m_pass2Levels[0].output
// 출력:
//   gSortedParticles[N]  — cellId 순으로 정렬된 결과
// 스크래치:
//   gScatterCounter[M]   — 각 cell 의 write cursor. 매 dispatch 전에 0 으로 초기화 필수!
//                          (cellStart 를 직접 mutate 하면 다음 frame neighbor query 가 깨짐)
//
// Dispatch: ceil(sphMaxParticleCount / GROUP_SIZE) — 셰이더가 gCB.particleCount 로 early-exit.

#define HLSL
#include "SPH/Particle.h"

RWStructuredBuffer<SPHParticle> gParticles : register(u0);
RWStructuredBuffer<uint> gScatterCounter : register(u1);
RWStructuredBuffer<SPHParticle> gSortedParticles : register(u2);
RWStructuredBuffer<uint> gSortedIndices : register(u3);

StructuredBuffer<uint> gParticleCellId : register(t0);
StructuredBuffer<uint> gCellStart : register(t1);

ConstantBuffer<SPHParticleLocalConstant> gCB : register(b0);

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= gCB.particleCount)
        return;

    uint cellId = gParticleCellId[DTid.x];

    // 같은 cell 안에서의 상대 슬롯 — InterlockedAdd 가 unique 한 localSlot 반환.
    uint localSlot;
    InterlockedAdd(gScatterCounter[cellId], 1, localSlot);

    uint dst = gCellStart[cellId] + localSlot;
    gSortedParticles[dst] = gParticles[DTid.x];
    gSortedIndices[dst] = DTid.x;
}
