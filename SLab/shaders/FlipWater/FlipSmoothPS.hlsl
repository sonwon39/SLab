// FLIP SSF separable filter. Shares the fullscreen VS (FlipBackgroundVS -> FsOut).
//   gPass.y == 0 : narrow-range depth filter (simplified from Truong & Yuksel,
//                  "A Narrow-Range Filter for Screen-Space Fluid Rendering",
//                  i3D 2018). Neighbors outside [d0-delta, d0+delta] are clamped
//                  into the range so distant surfaces cannot bleed across
//                  silhouettes while the surface still smooths into a sheet.
//   gPass.y == 1 : plain gaussian (thickness blur).
//   gPass.x      : 0 = horizontal, 1 = vertical.
// The pixel radius adapts to depth: a fixed world radius projected to pixels.
#include "FlipCommon.hlsli"

Texture2D srcTex : register(t0);

float main(FsOut i) : SV_Target
{
    int2 pix = int2(i.pos.xy);
    float d0 = srcTex.Load(int3(pix, 0)).r;
    bool plain = (gPass.y == 1);
    float zfarHalf = gFrame.fluidA.w * 0.5;
    if (!plain && d0 > zfarHalf) return d0;                  // background pixel

    float radius;
    if (plain)
    {
        radius = 9.0;
    }
    else
    {
        float pxPerWorld = gFrame.projA.y * gFrame.screen.y * 0.5 / d0;
        radius = clamp(gFrame.fluidB.x * pxPerWorld, 1.0, gFrame.fluidB.z);
    }
    float sigma = radius / 2.5;
    float thresh = gFrame.fluidB.y;                          // world-depth delta
    int2 dir = (gPass.x == 1) ? int2(0, 1) : int2(1, 0);

    float sum = d0, wsum = 1.0;
    int R = (int)radius;
    [loop] for (int s = -R; s <= R; s++)
    {
        if (s == 0) continue;
        float d = srcTex.Load(int3(pix + dir * s, 0)).r;
        if (!plain)
        {
            if (d > zfarHalf) continue;                     // keep silhouettes crisp
            d = clamp(d, d0 - thresh, d0 + thresh);         // the narrow range
        }
        float w = exp(-(float)(s * s) / (2.0 * sigma * sigma));
        sum += d * w;
        wsum += w;
    }
    return sum / wsum;
}
