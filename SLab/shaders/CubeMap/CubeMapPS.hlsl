#include "CubeMapCommon.hlsli"

float4 main(PSInput input) : SV_TARGET
{
    return g_cubeMap.SampleLevel(g_wrapLinearSampler, input.worldPos, 0.f);
}
