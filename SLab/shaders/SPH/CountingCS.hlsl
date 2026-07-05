#include "CountingSortCommon.hlsli"

RWStructuredBuffer<SPHParticle> particles : register(u0);
RWStructuredBuffer<uint> gCellCounter : register(u1);
RWStructuredBuffer<uint> gParticleCellId : register(u2);

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gParticleLocalCB.particleCount <= DTid.x)
        return;

    float3 p = particles[DTid.x].position;
    uint cellId = CellIdFromPos(p);
    gParticleCellId[DTid.x] = cellId;

    uint _;
    InterlockedAdd(gCellCounter[cellId], 1, _);
}
