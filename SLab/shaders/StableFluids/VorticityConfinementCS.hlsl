#include "StableFluidsUtility.hlsli"

Texture2D<float> gCurl : register(t0);
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

    float leftW = gCurl[left];
    float rightW = gCurl[right];
    float upW = gCurl[up];
    float downW = gCurl[down];

    float w = gCurl[DTid.xy];

    float2 dx = float2(1.f / gridDim.x, 1.f / gridDim.y);
    float2 curlDivergence = float2(abs(rightW - leftW) / (2.f * dx.x), abs(upW - downW) / (2.f * dx.y));

    if (length(curlDivergence) < 1e-5)
        return;

    float3 N = float3(normalize(curlDivergence), 0.f);
    float3 omega = float3(0.f, 0.f, w);

    float e = 0.2f;

    float2 vorticity = e * cross(N, omega).xy * dx;

    gVelocity[DTid.xy].xy += vorticity;
}
