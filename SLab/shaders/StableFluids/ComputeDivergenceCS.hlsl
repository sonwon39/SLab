#include "StableFluidsUtility.hlsli"

Texture2D<float4> gVelocity : register(t0);
RWTexture2D<float> gDivergence : register(u0);
RWTexture2D<float> gPressure0 : register(u1);
RWTexture2D<float> gPressure1 : register(u2);

SamplerState gWrapLinearSampler : register(s0);

[numthreads(SF_GROUP_SIZE_X, SF_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint3 gridDim = gLocalCB.gGridDim;

    if (DTid.x >= gridDim.x || DTid.y >= gridDim.y)
        return;

    float deltaTime = gLocalCB.deltaTime;

    float divergence = 0.f;

    uint2 left = uint2(DTid.x == 0 ? gridDim.x - 1 : DTid.x - 1, DTid.y);
    uint2 right = uint2(DTid.x == gridDim.x - 1 ? 0 : DTid.x + 1, DTid.y);
    uint2 up = uint2(DTid.x, DTid.y == gridDim.y - 1 ? 0 : DTid.y + 1);
    uint2 down = uint2(DTid.x, DTid.y == 0 ? gridDim.y - 1 : DTid.y - 1);

    float2 leftVel = gVelocity[left].xy;
    float2 rightVel = gVelocity[right].xy;
    float2 upVel = gVelocity[up].xy;
    float2 downVel = gVelocity[down].xy;

    divergence = (rightVel.x - leftVel.x + upVel.y - downVel.y) * 0.5f;
    gDivergence[DTid.xy] = divergence;

    gPressure0[DTid.xy] = 0.f;
    gPressure1[DTid.xy] = 0.f;
}
