// Pass2a: per-threadgroup exclusive scan + emit per-block total.
//
// Re-used at every scan level. SPH::Pass2 가 매 level의 bind/dispatch를 갈아끼움.
//   Level 0:  in = cellCounter    , out = cellStart            , blockSums = bs_L0
//   Level k:  in = bs_{k-1}       , out = m_pass2Levels[k].output, blockSums = bs_k
//
// Dispatch:  ceil(scanCount / GROUP_SIZE)
//
// Wave intrinsic (SM 6.0+) 사용. lane 0 이 각 wave의 합계를 groupshared 에 저장 →
// wave 0 이 그 합계들을 한 번 더 scan → 각 thread는 (wave offset + wave prefix) 합산.
// 한 group 안 thread 수가 GROUP_SIZE 라 wave 수 <= laneCount 보장 → 1-pass groupshared 로 충분.

#define HLSL
#include "SPH/Particle.h"

#define MAX_WAVES_PER_GROUP (GROUP_SIZE / 32) // wave32 hardware 기준 상한

StructuredBuffer<uint> gInput : register(t0);
RWStructuredBuffer<uint> gOutput : register(u0);    // 이 level의 block-내부 exclusive scan
RWStructuredBuffer<uint> gBlockSums : register(u1); // 각 block의 inclusive 총합

ConstantBuffer<SPHParticleLocalConstant> gCB : register(b0);

groupshared uint sWaveSums[MAX_WAVES_PER_GROUP];

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID,
                                         uint GTid : SV_GroupIndex)
{
    const uint laneCount = WaveGetLaneCount();
    const uint laneIdx = WaveGetLaneIndex();
    const uint waveIdx = GTid / laneCount;
    const uint waveCount = GROUP_SIZE / laneCount;

    const uint scanCount = gCB.gScanCount;

    // OOB lane은 0으로 채워 마지막 partial block 도 올바르게 계산.
    uint val = (DTid.x < scanCount) ? gInput[DTid.x] : 0u;

    // 1) wave 안 exclusive scan + wave 합계 (각 1 instruction)
    uint wavePrefix = WavePrefixSum(val);
    uint waveTotal = WaveActiveSum(val);

    if (laneIdx == 0)
        sWaveSums[waveIdx] = waveTotal;
    GroupMemoryBarrierWithGroupSync();

    // 2) wave 0 이 wave 합계들을 scan (waveCount <= laneCount 가정)
    if (waveIdx == 0)
    {
        uint x = (laneIdx < waveCount) ? sWaveSums[laneIdx] : 0u;
        uint scanned = WavePrefixSum(x);
        if (laneIdx < waveCount)
            sWaveSums[laneIdx] = scanned;
    }
    GroupMemoryBarrierWithGroupSync();

    // 3) 합산: 내 thread의 block-내부 exclusive prefix = wave offset + wave 내부 prefix
    uint blockExclusive = sWaveSums[waveIdx] + wavePrefix;

    if (DTid.x < scanCount)
        gOutput[DTid.x] = blockExclusive;

    // 4) block의 마지막 active lane이 block 총합 (inclusive)을 기록.
    //    Full block: GTid == GROUP_SIZE-1.
    //    Tail block: OOB lane은 val=0 이므로 동일 코드로 정확.
    if (GTid == GROUP_SIZE - 1)
    {
        gBlockSums[Gid.x] = blockExclusive + val;
    }
}
