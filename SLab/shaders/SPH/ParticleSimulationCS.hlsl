#include "ParticleCommon.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
ConstantBuffer<ParticleLocalConstant> gLocalCB : register(b0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 pos = particles[DTid.x].pos;
    float3 nextPos = mul(float4(pos, 1.f), gLocalCB.model).xyz;
    particles[DTid.x].pos = nextPos;
    particles[DTid.x].color = float3(1.f, 0.f, 0.f);
}
