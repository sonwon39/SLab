#include "DefaultCommon.hlsli"

float4 main(PSInput input) : SV_TARGET
{
    return float4(g_albedo.Sample(g_wrapLinearSampler, input.uv).xyz, 1.f);
}
