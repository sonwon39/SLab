#include "SPHUtility.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint i = DTid.x;
    if (i >= gParticleLocalCB.particleCount)
        return;

    float3 xi = prev_particles[i].position;
    int3 gridDim = int3(gParticleLocalCB.gGridDim);

    int3 cellXYZ = int3(floor((xi - gParticleLocalCB.gGridMin) / gParticleLocalCB.gCellSize));
    cellXYZ = clamp(cellXYZ, int3(0, 0, 0), gridDim - 1);

    float density = 0.0f;

    for (int dz = -1; dz <= 1; dz++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
            {
                int3 nc = cellXYZ + int3(dx, dy, dz);
                if (any(nc < 0) || any(nc >= gridDim))
                    continue;

                uint cellId = LinearCellId(nc);
                uint cellStart = gCellStart[cellId];
                uint cellCount = gCellCounter[cellId];

                for (uint j = 0; j < cellCount; j++)
                {
                    uint id = cellStart + j;
                    uint pId = gSortedIndices[id];
                    if (pId >= gParticleLocalCB.particleCount)
                        continue;
                    float3 xj = sorted_particles[id].position;
                    density += W(xi, xj);
                }
            }

    curr_particles[i].density = density;
}
