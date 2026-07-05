#include "ParticleCommon.hlsli"

StructuredBuffer<SPHParticle> particles : register(t0);

GSInput main(uint id : SV_VertexID)
{
    GSInput output;
    output.pos = particles[id].position;

    // float t = float(id.x) / float(gParticleLocalCB.particleCount);
    // float3 color = float3(t, 1.0 - t, 0.5);

    output.color = particles[id].color;
    output.radius = particles[id].radius;

    return output;
}
