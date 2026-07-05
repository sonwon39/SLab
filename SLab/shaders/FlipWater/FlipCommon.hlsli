// Shared declarations for the FLIP SSF render shaders (billboard sphere impostor,
// depth, thickness, smoothing, composite). Uses the per-frame FlipFrameConstant.
#define HLSL
#include "FlipWater/FlipParticle.h"

ConstantBuffer<FlipFrameConstant> gFrame : register(b0);

// per-pass root 32-bit constants: gPass.x = view mode / scene id,
// gPass.y = filter direction (0=X,1=Y) or variant, gPass.z/w spare.
cbuffer FlipPassCB : register(b1) { uint4 gPass; }

SamplerState sLinear : register(s0);   // clamp-linear (SSF filtering / refraction)

// billboard sphere-impostor interpolants (a camera-facing quad shaded as a sphere)
struct SpriteOut
{
    float4 pos : SV_Position;
    float2 q : TEXCOORD0;       // corner in [-1,1]^2
    float centerZ : TEXCOORD1;  // sphere center view-space depth
    float speed : TEXCOORD2;
    float3 color : TEXCOORD3;
};

// 6 corners = 2 triangles per particle (no vertex/index buffer)
static const float2 kCorners[6] = {
    float2(-1, -1), float2(1, -1), float2(-1, 1),
    float2(1, -1),  float2(1, 1),  float2(-1, 1)
};

// fullscreen-triangle interpolants (screen-space passes)
struct FsOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

float3 skyColor(float3 rd)
{
    float t = saturate(rd.y * 0.5 + 0.5);
    float3 sky = lerp(float3(0.82, 0.90, 0.97), float3(0.23, 0.46, 0.78), pow(t, 0.7));
    float s = saturate(dot(rd, normalize(gFrame.sunDir.xyz)));
    sky += float3(1.0, 0.90, 0.70) * pow(s, 800.0) * 6.0;  // sun disc
    sky += float3(1.0, 0.95, 0.80) * pow(s, 8.0) * 0.12;   // halo
    return sky;
}

float3 gammaOut(float3 c) { return pow(max(c, 0.0), 1.0 / 2.2); }
