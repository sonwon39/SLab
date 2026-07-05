#define HLSL
#include "GlobalConstant.h"
#include "PBR/PBRHLSLCompat.h"

TextureCube g_cubeMap : register(t0);
ConstantBuffer<GlobalConstant> gGlobalCB : register(b0);
ConstantBuffer<LocalConstant> gLocalCB : register(b1);

SamplerState g_wrapLinearSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 svPosition : SV_Position;
    float3 worldPos : POSTION;
};
