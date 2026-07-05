#include "NoiseCommon.hlsli"

StructuredBuffer<NoiseParticle> gParticles : register(t0);
RWTexture2D<float4>			    gDensity   : register(u0);

ConstantBuffer<NoiseLocalConstant> gLCB : register(b0);

SamplerState gWrapLinearSampler : register(s0);

[numthreads(N_GROUP_SIZE_X, N_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint width;
	uint height;
	gDensity.GetDimensions(width, height);
	if (DTid.x >= width || DTid.y >= height)
		return;

	float dt = gLCB.deltaTime;
	//gDensity[DTid.xy].rgb = max(0.f, gDensity[DTid.xy].rgb - dt * 10.f);
	//gDensity[DTid.xy].rgb = max(0.f, gDensity[DTid.xy].rgb - dt);
	gDensity[DTid.xy].rgb = max(0.f, gDensity[DTid.xy].rgb - dt * 2.f);
	float2 texel = float2(1.f / width, 1.f / height);
	float2 uv = DTid.xy * texel;
	
	for (int i = 0; i < gLCB.particleCount; i++)
	{
		NoiseParticle p = gParticles[i];
		float2 pos = p.position.xy;
		float r = p.radius;
		float scale = 1.f - (length(uv - pos) / r);
		gDensity[DTid.xy].rgb += smoothstep(0.f, 1.f, scale) * p.color;
	}
}
