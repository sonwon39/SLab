// FLIP SSF depth pass: carve the billboard quad into a sphere and write the
// sphere front-face LINEAR view depth to R32F, plus the matching hardware depth
// (SV_Depth) so the D32 buffer keeps only the nearest particle per pixel.
// Shares the billboard VS (FlipParticleVS -> SpriteOut).
#include "FlipCommon.hlsli"

struct DepthOut
{
    float d : SV_Target;    // linear view depth -> depthRaw (R32F)
    float dev : SV_Depth;   // hardware depth (nearest-surface test)
};

DepthOut main(SpriteOut i)
{
    float r2 = dot(i.q, i.q);
    if (r2 > 1.0) discard;                                   // carve disc into sphere
    float nz = sqrt(1.0 - r2);
    float z = max(i.centerZ - nz * gFrame.fluidA.x, gFrame.fluidA.z + 1e-3);

    DepthOut o;
    o.d = z;
    o.dev = gFrame.projA.z + gFrame.projA.w / z;             // (z*P33 + P43) / z
    return o;
}
