#include "CubeMapCommon.hlsli"

PSInput main(VSInput input)
{
    PSInput output;

    float4 pos = mul(float4(input.position, 0.f), gGlobalCB.view);
    pos = mul(float4(pos.xyz, 1.f), gGlobalCB.projection);
    output.svPosition = pos.xyww;
    output.worldPos = input.position;

    return output;
}
