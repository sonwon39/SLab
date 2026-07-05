#include "CountingSortCommon.hlsli"

RWStructuredBuffer<uint> gCellCounter : register(u0);
RWStructuredBuffer<uint> gScatterCounter : register(u1);

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gParticleLocalCB.gCellCount <= DTid.x)
        return;

    gCellCounter[DTid.x] = 0u;
    gScatterCounter[DTid.x] = 0u;
}
