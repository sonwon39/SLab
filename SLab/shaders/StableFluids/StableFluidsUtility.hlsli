#define HLSL
#include "StableFluids/Grid.h"

ConstantBuffer<SFLocalConstant> gLocalCB : register(b0);

uint FlatIdx(int3 c)
{
    uint3 dim = gLocalCB.gGridDim;
    return c.x + c.y * dim.x + c.z * dim.x * dim.y;
}

uint3 UnflattenIdx(int idx)
{
    uint3 gridDim = gLocalCB.gGridDim;
    uint xyArea = gridDim.x * gridDim.y;
    uint iz = idx / xyArea;
    uint xy = idx % xyArea;
    uint iy = xy / gridDim.x;
    uint ix = xy % gridDim.x;

    return uint3(ix, iy, iz);
}
