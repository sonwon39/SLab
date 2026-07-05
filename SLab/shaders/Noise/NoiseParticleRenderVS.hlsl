#include "NoiseCommon.hlsli"

StructuredBuffer<NoiseParticle> particles : register(t0);

GSInput main(uint id : SV_VertexID)
{
    GSInput output;
    output.pos = particles[id].position;
    output.radius = particles[id].radius;

    return output;
}
