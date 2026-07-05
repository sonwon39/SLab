#include "SPHUtility.hlsli"

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int i = DTid.x;
    if (i >= gParticleLocalCB.particleCount)
        return;

    float rhoi = curr_particles[i].density;
    curr_particles[i].pressure = max(0.f, gParticleLocalCB.k * (pow(rhoi / gParticleLocalCB.rho0, 7) - 1.f));
}
