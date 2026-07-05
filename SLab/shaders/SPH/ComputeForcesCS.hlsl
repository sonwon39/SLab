#include "SPHUtility.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int i = DTid.x;
    if (i >= gParticleLocalCB.particleCount)
        return;

    float3 acc = float3(0.f, 0.f, 0.f);
    float rhoi = curr_particles[i].density;
    float rhoi_2 = rhoi * rhoi;
    float pi = curr_particles[i].pressure;
    float3 xi = prev_particles[i].position;
    float3 vi = prev_particles[i].velocity;

    float h = gParticleLocalCB.h;
    float dt = gParticleLocalCB.dt;
    float mu = gParticleLocalCB.mu;

    int3 gridDim = int3(gParticleLocalCB.gGridDim);
    int3 cellXYZ = int3(floor((xi - gParticleLocalCB.gGridMin) / gParticleLocalCB.gCellSize));
    cellXYZ = clamp(cellXYZ, int3(0, 0, 0), gridDim - 1);

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

                    float3 xj = prev_particles[pId].position;
                    float3 xij = xi - xj;

                    float3 vj = prev_particles[pId].velocity;
                    float3 vij = vi - vj;

                    float3 w_grad = W_Grad(xi, xj);

                    float rhoj = curr_particles[pId].density;
                    float rhoj_2 = rhoj * rhoj;
                    float pj = curr_particles[pId].pressure;

                    float3 pressure_acc = -((pi / rhoi_2) + (pj / rhoj_2)) * w_grad;
                    float3 viscosity_acc =
                        2 * (mu / (rhoi * rhoj)) * vij * dot(xij, w_grad) / (dot(xij, xij) + 0.01 * h * h);
                    acc += pressure_acc + viscosity_acc;
                }
            }
    float3 gravity_acc = float3(0.f, -9.8f, 0.f);

    curr_particles[i].acceleration = acc + gravity_acc;
    // curr_particles[i].acceleration = gravity_acc;
}
