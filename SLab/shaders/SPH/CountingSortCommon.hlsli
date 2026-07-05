#define HLSL
#include "SPH/Particle.h"

#define MAX_WAVES_PER_GROUP (GROUP_SIZE / 32) // upper bound for wave32 hardware

ConstantBuffer<SPHParticleLocalConstant> gParticleLocalCB : register(b0);
// cbuffer GridCB : register(b0)
//{
//     float3 gGridMin;        // world-space corner of the grid
//     float  gCellSize;       // h (uniform; equal to SPH support radius)
//     uint3  gGridDim;        // cells per axis
//     uint   gParticleCount;  // N
//     uint   gCellCount;      // M = gGridDim.x * y * z
//     uint   gScanCount;      // length of input to current scan dispatch (varies per level)
//     uint   _pad0;
//     uint   _pad1;
// };

uint LinearCellId(int3 c)
{
    return uint(c.x) + uint(c.y) * uint(gParticleLocalCB.gGridDim.x) +
           uint(c.z) * uint(gParticleLocalCB.gGridDim.x) * uint(gParticleLocalCB.gGridDim.y);
}

uint CellIdFromPos(float3 p)
{
    int3 c = int3(floor((p - gParticleLocalCB.gGridMin) / gParticleLocalCB.gCellSize));
    c = clamp(c, int3(0, 0, 0), int3(gParticleLocalCB.gGridDim) - 1);
    return LinearCellId(c);
}
