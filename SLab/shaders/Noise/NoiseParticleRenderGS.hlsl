#include "NoiseCommon.hlsli"
#include "GlobalConstant.h"

StructuredBuffer<NoiseParticle> particles : register(t0);
ConstantBuffer<GlobalConstant> gGlobalCB : register(b0);

[maxvertexcount(6)]
void main(point GSInput input[1], inout TriangleStream<PSInput> output)
{
    float3 center = input[0].pos;
    float r = input[0].radius;
	
	float3 p0 = center + float3(r, -r, 0.f);
	float3 p1 = center + float3(-r, -r, 0.f);
	float3 p2 = center + float3(-r, r, 0.f);
	float3 p3 = center + float3(r, r, 0.f);
	
	float3 points[4] = { p0, p1, p2, p3 };
	uint indices[6] = { 0, 1, 2, 0, 2, 3 };

	PSInput v;
	v.radius = r;
	
	for (int i = 0; i < 6; i++)
	{
		uint index = indices[i];
		float3 p = points[index];
		float4 svPos = mul(float4(p, 1.f), gGlobalCB.view);
		svPos = mul(svPos, gGlobalCB.projection);
		v.pos = svPos;
		output.Append(v);
		if ((i + 1) % 3 == 0)
			output.RestartStrip();
	}
}
