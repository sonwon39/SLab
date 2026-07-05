#include "StableFluidsUtility.hlsli"

Texture2D<float> gPressure : register(t0);
RWTexture2D<float4> gVelocity : register(u0);

SamplerState gWrapLinearSampler : register(s0);

[numthreads(SF_GROUP_SIZE_X, SF_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint3 gridDim = gLocalCB.gGridDim;

    if (DTid.x >= gridDim.x || DTid.y >= gridDim.y)
        return;

    uint2 left = uint2(DTid.x == 0 ? gridDim.x - 1 : DTid.x - 1, DTid.y);
    uint2 right = uint2(DTid.x == gridDim.x - 1 ? 0 : DTid.x + 1, DTid.y);
    uint2 up = uint2(DTid.x, DTid.y == gridDim.y - 1 ? 0 : DTid.y + 1);
    uint2 down = uint2(DTid.x, DTid.y == 0 ? gridDim.y - 1 : DTid.y - 1);

    float leftP = gPressure[left];
    float rightP = gPressure[right];
    float upP = gPressure[up];
    float downP = gPressure[down];

    float dx = 2.f;
    float2 divergenceP = float2(rightP - leftP, upP - downP) * 0.5f;

    gVelocity[DTid.xy].xy -= divergenceP;
}
