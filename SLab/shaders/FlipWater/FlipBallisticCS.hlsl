// FLIP build-along STEP 1: gravity only, no grid.
// Each particle is independent: integrate gravity (explicit Euler) and clamp to
// the box walls. Particles never see each other, so they interpenetrate and pile
// into a flat sheet on the floor - the visible "dust" that motivates adding a
// shared MAC grid (P2G/G2P) in step 2.
#define HLSL
#include "FlipWater/FlipParticle.h"

RWStructuredBuffer<FlipParticle> particles : register(u0);
ConstantBuffer<FlipConstant> gFlip : register(b0);

[numthreads(FLIP_GROUP_SIZE, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint i = tid.x;
    if (i >= gFlip.particleCount) return;

    FlipParticle p = particles[i];

    p.velocity += gFlip.gravity * gFlip.dt;   // gravity
    p.position += p.velocity * gFlip.dt;      // explicit Euler advection

    // clamp to box walls; kill the velocity component that hit a wall (settle)
    if (p.position.x < gFlip.boundsMin.x) { p.position.x = gFlip.boundsMin.x; p.velocity.x = max(p.velocity.x, 0.0); }
    if (p.position.x > gFlip.boundsMax.x) { p.position.x = gFlip.boundsMax.x; p.velocity.x = min(p.velocity.x, 0.0); }
    if (p.position.y < gFlip.boundsMin.y) { p.position.y = gFlip.boundsMin.y; p.velocity.y = max(p.velocity.y, 0.0); }
    if (p.position.y > gFlip.boundsMax.y) { p.position.y = gFlip.boundsMax.y; p.velocity.y = min(p.velocity.y, 0.0); }
    if (p.position.z < gFlip.boundsMin.z) { p.position.z = gFlip.boundsMin.z; p.velocity.z = max(p.velocity.z, 0.0); }
    if (p.position.z > gFlip.boundsMax.z) { p.position.z = gFlip.boundsMax.z; p.velocity.z = min(p.velocity.z, 0.0); }

    particles[i] = p;
}
