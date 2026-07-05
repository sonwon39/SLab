// Pass2b: scanned block-sum 을 각 block의 모든 원소에 더해 이 level의 exclusive scan 을
// 전역(globally-correct)으로 완성한다.
//
// Dispatch:  ceil(scanCount / GROUP_SIZE)   (Pass2a 와 동일한 모양)
// 입력:      gPartial[i]            = ScanBlocksCS 가 만든 block-내부 exclusive scan
//            gScannedBlockSums[Gid] = 한 단계 깊은 재귀 level의 output
//                                   = 이 level 의 모든 이전 block 의 inclusive 합 (=전역 prefix)
// 출력:      gPartial[i] += gScannedBlockSums[Gid]

#define HLSL
#include "SPH/Particle.h"

RWStructuredBuffer<uint> gPartial : register(u0);
StructuredBuffer<uint> gScannedBlockSums : register(t0);

ConstantBuffer<SPHParticleLocalConstant> gCB : register(b0);

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID)
{
    if (DTid.x >= gCB.gScanCount)
        return;
    gPartial[DTid.x] += gScannedBlockSums[Gid.x];
}
