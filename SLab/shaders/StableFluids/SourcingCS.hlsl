#include "StableFluidsUtility.hlsli"
#include "GlobalConstant.h"

RWTexture2D<float4> gDensity : register(u0);
RWTexture2D<float4> gVelocity : register(u1);

ConstantBuffer<MouseConstant> gMouse : register(b1);

float smootherstep(float x, float edge0 = 0.0f, float edge1 = 1.0f)
{
    x = clamp((x - edge0) / (edge1 - edge0), 0, 1);
    return x * x * x * (3 * x * (2 * x - 5) + 10.0f);
}

[numthreads(SF_GROUP_SIZE_X, SF_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float brushRadius = 50.f;
    uint width, height;
    gDensity.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    gDensity[DTid.xy] = max(0.0, gDensity[DTid.xy] - 0.001);

	float deltaTime = gLocalCB.deltaTime;
	float sigma = brushRadius / 2.5f;
	float2 pixelPos = DTid.xy;

	float2 sourcingPos1 = float2(100.f, height / 2.f);
	float2 velocity1 = float2(0.1f, 0.f);

	float d1 = distance(pixelPos, sourcingPos1) / brushRadius;
	float scale1 = smootherstep(1.0 - d1);

	//float2 sourcingPos2 = float2(width - 100.f, height / 2.f);
	//float2 velocity2 = float2(-0.1f, 0.f);

	//float d2 = distance(pixelPos, sourcingPos2) / brushRadius;
	//float scale2 = smootherstep(1.0 - d2);

	float3 color = gMouse.color;
	//gDensity[DTid.xy].xyz += (scale1 * color);
	//gVelocity[DTid.xy].xy += (scale1 * velocity1);
	

    if (gMouse.lButtonDown)
    {
        float2 mouseCurrPos = float2(gMouse.posX, gMouse.posY);
        float2 velocity = gMouse.velocity;

        float d = distance(pixelPos, mouseCurrPos) / brushRadius;
        float scale = smootherstep(1.0 - d);

        color = gMouse.color;
        gDensity[DTid.xy].xyz += scale * color;
        gVelocity[DTid.xy].xy += scale * velocity;
    }
}
