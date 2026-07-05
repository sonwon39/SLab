#include "StableFluidsUtility.hlsli"

Texture2D<float> gDivergence : register(t0);
Texture2D<float> gOldPressure : register(t1);
RWTexture2D<float> gNewPressure : register(u0);

SamplerState gWrapLinearSampler : register(s0);

[numthreads(SF_GROUP_SIZE_X, SF_GROUP_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint3 gridDim = gLocalCB.gGridDim;

    if (DTid.x >= gridDim.x || DTid.y >= gridDim.y)
        return;

    if (DTid.x == 0 && DTid.y == 0)
    {
        gNewPressure[DTid.xy] = 0.f;
        return;
    }

    uint2 left = uint2(DTid.x == 0 ? gridDim.x - 1 : DTid.x - 1, DTid.y);
    uint2 right = uint2(DTid.x == gridDim.x - 1 ? 0 : DTid.x + 1, DTid.y);
    uint2 up = uint2(DTid.x, DTid.y == gridDim.y - 1 ? 0 : DTid.y + 1);
    uint2 down = uint2(DTid.x, DTid.y == 0 ? gridDim.y - 1 : DTid.y - 1);

    float leftP = gOldPressure[left];
    float rightP = gOldPressure[right];
    float upP = gOldPressure[up];
    float downP = gOldPressure[down];

    float divergence = gDivergence[DTid.xy];

    float pressure = (leftP + rightP + upP + downP - divergence) * 0.25f;

    gNewPressure[DTid.xy] = pressure;
}
