// Fullscreen-triangle VS shared by the screen-space passes (background, smooth,
// composite). Bufferless: 3 verts from SV_VertexID -> FsOut (pos + uv).
#include "FlipCommon.hlsli"

FsOut main(uint id : SV_VertexID)
{
    FsOut o;
    o.uv = float2((id << 1) & 2, id & 2);          // (0,0) (2,0) (0,2)
    o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}
