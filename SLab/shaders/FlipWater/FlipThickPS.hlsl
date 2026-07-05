// FLIP SSF thickness pass: additively accumulate each sphere's view-ray chord
// length into R16F (with additive blend). Particles hidden behind the tank
// wall/floor are rejected via the scene view-depth stored in sceneColor.a.
// Shares the billboard VS (FlipParticleVS -> SpriteOut).
#include "FlipCommon.hlsli"

Texture2D sceneTex : register(t0);   // rgb = tank color, a = linear view depth

float main(SpriteOut i) : SV_Target
{
    float r2 = dot(i.q, i.q);
    if (r2 > 1.0) discard;                                   // carve disc into sphere
    float sceneZ = sceneTex.Load(int3(i.pos.xy, 0)).a;
    if (i.centerZ > sceneZ) discard;                         // behind tank wall/floor
    float nz = sqrt(1.0 - r2);
    return 2.0 * nz * gFrame.fluidA.x * gFrame.fluidA.y;     // chord length * scale
}
