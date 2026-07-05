#include "NoiseCommon.hlsli"

Texture2D<float>   g_noise  : register(t0);
RWTexture2D<float4> g_curl	: register(u0);

SamplerState gWrapLinearSampler : register(s0);

[numthreads(N_GROUP_SIZE_X, N_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint x;
	uint y;
	g_curl.GetDimensions(x, y);
	if (DTid.x >= x || DTid.y >= y)
		return;

	float2 texel = float2(1.f /x, 1.f /y);
	float2 left = float2(DTid.x == 0 ? x - 1 : DTid.x - 1, DTid.y) + 0.5f;
	float2 right = float2(DTid.x == x - 1 ? 0 : DTid.x + 1, DTid.y) + 0.5f;
	float2 up = float2(DTid.x, DTid.y == y - 1 ? 0 : DTid.y + 1) + 0.5f;
	float2 down = float2(DTid.x, DTid.y == 0 ? y - 1 : DTid.y - 1) + 0.5f;
	
	float2 uv = DTid.xy / float2(x, y);
		
	float ln = g_noise.SampleLevel(gWrapLinearSampler, left * texel, 0.0);
	float rn = g_noise.SampleLevel(gWrapLinearSampler, right * texel, 0.0);
	float un = g_noise.SampleLevel(gWrapLinearSampler, up * texel, 0.0);
	float dn = g_noise.SampleLevel(gWrapLinearSampler, down * texel, 0.0);
	
	g_curl[DTid.xy].xy = float2(un - dn, -(rn - ln));
}
