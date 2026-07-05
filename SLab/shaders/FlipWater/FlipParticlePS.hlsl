// FLIP particle debug PS (viewmode 1): carve the quad into a sphere, shade by
// velocity (base color -> white when fast), output the sphere front-face depth
// so overlapping particles occlude correctly.
#include "FlipCommon.hlsli"

struct DebugOut
{
    float4 c : SV_Target;
    float dev : SV_Depth;
};

DebugOut main(SpriteOut i)
{
    float r2 = dot(i.q, i.q);
    if (r2 > 1.0) discard;                                   // carve disc into sphere
    float nz = sqrt(1.0 - r2);
    float z = max(i.centerZ - nz * gFrame.fluidA.x, gFrame.fluidA.z + 1e-3); // front-face view depth

    float3 n = float3(i.q, -nz);                             // view-space normal toward camera
    float t = saturate(i.speed / 12.0);
    float3 col = lerp(i.color, float3(1.0, 1.0, 1.0), smoothstep(0.6, 1.0, t));
    float3 L = normalize(float3(-0.4, 0.7, -0.6));
    float dif = saturate(dot(n, L)) * 0.7 + 0.4;

    DebugOut o;
    o.c = float4(gammaOut(col * dif), 1.0);                  // match gamma-corrected scene
    o.dev = gFrame.projA.z + gFrame.projA.w / z;             // linear -> hardware depth
    return o;
}
