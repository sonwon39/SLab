// FLIP tank background - analytic "pool room": the [0,1]^3 open tank seen as if
// the near glass wall were invisible (slab test, shade the EXIT face), an
// infinite warm-tiled ground outside, and a procedural sky above.
// Output: rgb = LINEAR color, a = LINEAR view depth (occlusion test + refraction
// source in Composite). Rendered to sceneColor (RGBA16F).
#include "FlipCommon.hlsli"

float3 poolTile(float2 q, float ao)
{
    float2 t = frac(q * 8.0);                      // 12.5 cm pool tiles
    float2 c = floor(q * 8.0);
    float checker = fmod(abs(c.x + c.y), 2.0);
    float3 col = lerp(float3(0.72, 0.86, 0.92), float3(0.55, 0.76, 0.88), checker);
    float2 g = min(t, 1.0 - t);
    float grout = smoothstep(0.0, 0.035, min(g.x, g.y));
    col = lerp(float3(0.42, 0.52, 0.58), col, grout);
    return col * ao;
}

float3 groundTile(float2 q)
{
    float2 c = floor(q * 2.0);                     // 50 cm slabs
    float checker = fmod(abs(c.x + c.y), 2.0);
    float3 col = lerp(float3(0.78, 0.74, 0.68), float3(0.70, 0.66, 0.60), checker);
    float2 t = frac(q * 2.0);
    float2 g = min(t, 1.0 - t);
    col *= lerp(0.75, 1.0, smoothstep(0.0, 0.02, min(g.x, g.y)));
    return col;
}

float4 main(FsOut i) : SV_Target
{
    float2 ndc = float2(i.uv.x * 2 - 1, 1 - 2 * i.uv.y);
    float3 rd = normalize(gFrame.camFwd.xyz + gFrame.camRight.xyz * ndc.x / gFrame.projA.x
                                            + gFrame.camUp.xyz    * ndc.y / gFrame.projA.y);
    float3 ro = gFrame.camPos.xyz;

    // slab test against the unit box
    float3 inv = 1.0 / rd;
    float3 t0 = (0.0 - ro) * inv, t1 = (1.0 - ro) * inv;
    float3 tmin3 = min(t0, t1), tmax3 = max(t0, t1);
    float tEnter = max(max(tmin3.x, tmin3.y), tmin3.z);
    float tExit  = min(min(tmax3.x, tmax3.y), tmax3.z);
    bool boxHit = (tExit >= max(tEnter, 0.0));

    float3 col;
    float depth = gFrame.fluidA.w;                 // default: far (sky)
    bool shaded = false;

    if (boxHit)
    {
        uint axis = (tExit == tmax3.x) ? 0u : (tExit == tmax3.y) ? 1u : 2u;
        if (!(axis == 1 && rd.y > 0.0))            // exit through top = open
        {
            float3 hp = ro + rd * tExit;
            if (axis == 1)                         // floor
            {
                float edge = min(min(hp.x, 1.0 - hp.x), min(hp.z, 1.0 - hp.z));
                float ao = 0.72 + 0.28 * smoothstep(0.0, 0.15, edge);
                col = poolTile(hp.xz, ao) * float3(1.06, 1.03, 0.98);
            }
            else                                   // side walls
            {
                float2 q = (axis == 0) ? hp.zy : hp.xy;
                float ao = 0.55 + 0.45 * saturate(hp.y * 1.4 + 0.15);
                col = poolTile(q, ao);
            }
            depth = dot(hp - ro, gFrame.camFwd.xyz);
            shaded = true;
        }
    }
    if (!shaded)
    {
        if (rd.y < -1e-4)                          // outside ground plane y=0
        {
            float t = -ro.y / rd.y;
            float3 hp = ro + rd * t;
            float fog = exp(-max(t - 1.5, 0.0) * 0.30);
            col = lerp(float3(0.80, 0.87, 0.94), groundTile(hp.xz), fog);
            depth = dot(hp - ro, gFrame.camFwd.xyz);
        }
        else
        {
            col = skyColor(rd);
        }
    }
    return float4(col, depth);
}
