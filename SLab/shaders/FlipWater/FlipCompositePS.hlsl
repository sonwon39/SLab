// FLIP SSF composite - the "make it look like water" pass. Shares the fullscreen
// VS (FlipBackgroundVS -> FsOut).
//   1. reconstruct view position + normal from the smoothed depth
//   2. refraction: offset UV by the normal, sample the tank, attenuate with
//      Beer-Lambert exp(-sigma*thickness) + inscatter
//   3. reflection: procedural sky along the reflected ray
//   4. blend by Schlick Fresnel (F0 = 0.02 water) + sun specular
// View modes (gPass.x): 1 scene only (debug spheres drawn after), 2 raw depth,
//   3 smoothed depth, 4 thickness, 5 normals, 6 shaded water.
#include "FlipCommon.hlsli"

Texture2D depthTex : register(t0);
Texture2D thickTex : register(t1);
Texture2D sceneTex : register(t2);

float3 viewPosAt(int2 pix, float z)
{
    float2 uv = ((float2)pix + 0.5) * gFrame.screen.zw;
    float2 ndc = float2(uv.x * 2 - 1, 1 - 2 * uv.y);
    return float3(ndc.x * z / gFrame.projA.x, ndc.y * z / gFrame.projA.y, z);
}
float depthAt(int2 pix) { return depthTex.Load(int3(pix, 0)).r; }

float4 main(FsOut i) : SV_Target
{
    int2 pix = int2(i.pos.xy);
    uint mode = gPass.x;
    float4 scene = sceneTex.Load(int3(pix, 0));
    float d = depthAt(pix);
    float zfarHalf = gFrame.fluidA.w * 0.5;

    if (mode == 2 || mode == 3)                              // depth visualization
    {
        float g = (d > zfarHalf) ? 0.0 : saturate(1.0 - (d - 1.0) / 2.5);
        return float4(g, g, g, 1);
    }
    if (mode == 4)                                           // thickness visualization
    {
        float th = thickTex.Load(int3(pix, 0)).r;
        float3 c = lerp(float3(0.0, 0.02, 0.06), float3(0.2, 0.7, 1.0), saturate(th * 2.0));
        c = lerp(c, float3(1, 1, 1), saturate(th * 2.0 - 1.0));
        return float4(c, 1);
    }

    bool fluid = (d < zfarHalf) && (d < scene.a + gFrame.fluidA.x);
    if (mode == 1 || !fluid)
        return float4(gammaOut(scene.rgb), 1);

    // -- normal reconstruction from smoothed depth --
    float3 pc = viewPosAt(pix, d);
    float dxp = depthAt(pix + int2(1, 0)), dxm = depthAt(pix - int2(1, 0));
    float dyp = depthAt(pix + int2(0, 1)), dym = depthAt(pix - int2(0, 1));
    float3 ddxv = (abs(dxp - d) < abs(dxm - d))
        ? viewPosAt(pix + int2(1, 0), dxp) - pc
        : pc - viewPosAt(pix - int2(1, 0), dxm);
    float3 ddyv = (abs(dyp - d) < abs(dym - d))
        ? viewPosAt(pix + int2(0, 1), dyp) - pc
        : pc - viewPosAt(pix - int2(0, 1), dym);
    float3 n = normalize(cross(ddyv, ddxv));
    if (n.z > 0) n = -n;                                     // face the camera (LH +z fwd)

    float3 nW = normalize(gFrame.camRight.xyz * n.x + gFrame.camUp.xyz * n.y + gFrame.camFwd.xyz * n.z);
    if (mode == 5) return float4(nW * 0.5 + 0.5, 1);

    float th = thickTex.Load(int3(pix, 0)).r;

    // -- refraction (screen-space UV offset, y flipped: view up vs uv down) --
    float2 uv = ((float2)pix + 0.5) * gFrame.screen.zw;
    float2 refrUV = uv + float2(n.x, -n.y) * gFrame.absorb.w * min(th, 1.5);
    refrUV = clamp(refrUV, 0.001, 0.999);
    float4 refrScene = sceneTex.SampleLevel(sLinear, refrUV, 0);
    if (refrScene.a < d) refrScene = scene;                 // don't refract foreground

    float3 T = exp(-gFrame.absorb.rgb * th);                // Beer-Lambert
    float3 inscatter = float3(0.01, 0.20, 0.28);            // deep-water glow
    float3 refr = refrScene.rgb * T + inscatter * (1.0 - T);

    // -- reflection + Fresnel + specular --
    float3 vpW = gFrame.camPos.xyz + gFrame.camRight.xyz * pc.x + gFrame.camUp.xyz * pc.y + gFrame.camFwd.xyz * pc.z;
    float3 viewW = normalize(vpW - gFrame.camPos.xyz);
    float3 rW = reflect(viewW, nW);
    float3 refl = skyColor(rW);
    float ct = saturate(dot(-viewW, nW));
    float F = 0.02 + 0.98 * pow(1.0 - ct, 5.0);
    float3 hv = normalize(normalize(gFrame.sunDir.xyz) - viewW);
    float spec = pow(saturate(dot(nW, hv)), 180.0) * 0.9;

    float3 col = lerp(refr, refl, F) + spec;
    return float4(gammaOut(col), 1);
}
