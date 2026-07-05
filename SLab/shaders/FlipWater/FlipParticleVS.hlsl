// FLIP particle billboard VS: expand each particle into a camera-facing quad
// (6 verts via SV_VertexID, no vertex/geometry shader). The PS carves it into a
// sphere impostor. Same technique as FlipWater (6 verts/particle, cheap for 768k).
#include "FlipCommon.hlsli"

StructuredBuffer<FlipParticle> gParticles : register(t4);   // root SRV (VS)

SpriteOut main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    FlipParticle p = gParticles[iid];
    float2 c = kCorners[vid];
    float r = gFrame.fluidA.x;                                // render radius (world)

    float3 vc = mul(float4(p.position, 1.0), gFrame.view).xyz; // view-space center
    float3 vpos = vc + float3(c * r, 0.0);                     // camera-facing quad (offset in view XY)

    SpriteOut o;
    o.pos = mul(float4(vpos, 1.0), gFrame.proj);
    o.q = c;
    o.centerZ = vc.z;
    o.speed = length(p.velocity);
    o.color = p.color;
    return o;
}
