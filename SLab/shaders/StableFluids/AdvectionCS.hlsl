#include "StableFluidsUtility.hlsli"

Texture2D<float4> gOldDensity : register(t0);
Texture2D<float4> gOldVelocity : register(t1);

RWTexture2D<float4> gNewDensity : register(u0);
RWTexture2D<float4> gNewVelocity : register(u1);

SamplerState gWarpLinearSampler : register(s0);

[numthreads(SF_GROUP_SIZE_X, SF_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint3 gridDim = gLocalCB.gGridDim;

    if (DTid.x >= gridDim.x || DTid.y >= gridDim.y)
        return;

    float dt = gLocalCB.deltaTime;

    float2 dx = float2(1.f / gridDim.x, 1.f / gridDim.y);
    float2 currPos = (DTid.xy + 0.5f) * dx;

    float2 vel = gOldVelocity.SampleLevel(gWarpLinearSampler, currPos, 0).xy;
    float2 prevPos = currPos - vel * dt;

    gNewVelocity[DTid.xy] = gOldVelocity.SampleLevel(gWarpLinearSampler, prevPos, 0);
    gNewDensity[DTid.xy] = gOldDensity.SampleLevel(gWarpLinearSampler, prevPos, 0);
}
