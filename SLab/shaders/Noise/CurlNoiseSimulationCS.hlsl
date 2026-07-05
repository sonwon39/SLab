#include "NoiseCommon.hlsli"

RWStructuredBuffer<NoiseParticle> gParticles : register(u0);

ConstantBuffer<NoiseLocalConstant> gLCB		 : register(b0);

SamplerState gWrapLinearSampler : register(s0);

[numthreads(SIMULATION_GROUP_SIZE_X, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= gLCB.particleCount)
		return;

	float dt = gLCB.deltaTime;
	float2 dx = float2(1.f / gLCB.grid.x, 1.f / gLCB.grid.y);
	//float2 del = getCurl(gParticles[DTid.x].position.xy, dx) * dt / 5.f;
	//float2 del = getCurl(gParticles[DTid.x].position.xy, dx) * dt / 10.f;
	float2 del = getCurl(gParticles[DTid.x].position.xy, dx) * dt / 5.f;

	
	gParticles[DTid.x].position.xy += del;
	gParticles[DTid.x].position.xy = gParticles[DTid.x].position.xy;

}
