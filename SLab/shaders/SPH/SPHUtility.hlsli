#define HLSL
#include "SPH/Particle.h"

RWStructuredBuffer<SPHParticle> prev_particles : register(u0);
RWStructuredBuffer<SPHParticle> curr_particles : register(u1);
RWStructuredBuffer<SPHParticle> sorted_particles : register(u2);
RWStructuredBuffer<uint> gSortedIndices : register(u3);

StructuredBuffer<uint> gCellStart : register(t0);
StructuredBuffer<uint> gCellCounter : register(t1);

ConstantBuffer<SPHParticleLocalConstant> gParticleLocalCB : register(b0);

uint LinearCellId(int3 c)
{
    return uint(c.x) + uint(c.y) * uint(gParticleLocalCB.gGridDim.x) +
           uint(c.z) * uint(gParticleLocalCB.gGridDim.x) * uint(gParticleLocalCB.gGridDim.y);
}

uint CellIdFromPos(float3 p)
{
    int3 c = int3(floor((p - gParticleLocalCB.gGridMin) / gParticleLocalCB.gCellSize));
    c = clamp(c, int3(0, 0, 0), int3(gParticleLocalCB.gGridDim) - 1);
    return LinearCellId(c);
}

float Kernel(float q)
{
    if (q >= 2)
    {
        return 0;
    }
    else
    {
        float ret = 0.f;
        if (q < 1)
        {
            ret = 2.f / 3.f - pow(q, 2) + 0.5f * pow(q, 3);
        }
        else
        {
            ret = pow((2 - q), 3) / 6.f;
        }
        return ret * gParticleLocalCB.kernelCoefficient;
    }
}

float Kernel_Grad(float q)
{
    if (q >= 2)
    {
        return 0;
    }
    else
    {
        float ret = 0.f;
        if (q < 1)
        {
            ret = -2.f * q + (3.f / 2.f) * pow(q, 2);
        }
        else
        {
            ret = -pow((2 - q), 2) * 0.5f;
        }
        return ret * gParticleLocalCB.kernelCoefficient;
    }
}
float W(float3 xi, float3 xj)
{
    float q = distance(xi, xj) / gParticleLocalCB.h;
    float f = Kernel(q);

    return gParticleLocalCB.hd * f;
}
// pressure
float3 W_Grad(float3 xi, float3 xj)
{
    float d = max(distance(xi, xj), 1e-6f);
    float q = distance(xi, xj) / gParticleLocalCB.h;
    float3 grad_r = (xi - xj) / d;
    float3 grad_q = (grad_r) / gParticleLocalCB.h;
    float f_prim = Kernel_Grad(q);

    return f_prim * gParticleLocalCB.hd * grad_q;
}
