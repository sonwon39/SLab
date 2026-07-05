#ifndef PARTICLE_H
#define PARTICLE_H

#ifdef HLSL
#include "../HlslCompat.h"
#else
using namespace DirectX::SimpleMath;
using namespace DirectX;
#endif

#define GROUP_SIZE 256

struct Particle
{
    XMFLOAT3 pos;
    XMFLOAT3 color;
};

struct SPHParticle
{
    XMFLOAT3 position;
    float density;
    XMFLOAT3 color;
    float pressure;
    XMFLOAT3 velocity;
    float radius;
    XMFLOAT3 acceleration;
};

// struct Grid {
//	XMFLOAT3 gGridMin;      // world-space corner of the grid
//	float    dummy;
//	XMFLOAT3 gGridMax;
//	float  gCellSize;       // h (uniform; equal to SPH support radius)
//	XMFLOAT3  gGridDim;     // cells per axis
//	UINT   gParticleCount;  // N
//	UINT   gCellCount;      // M = gGridDim.x * y * z
//	UINT   gScanCount;
// };

struct ParticleLocalConstant
{
    Matrix model;
};

struct SPHParticleLocalConstant
{
    UINT particleCount;
    float kernelCoefficient;
    float h;
    float hd; // 1 / pow(h, d)

    float rho0;
    float k;
    float dt;
    float mu;

    XMFLOAT3 gGridMin; // world-space corner of the grid
    UINT gCellCount;   // M = gGridDim.x * y * z

    XMFLOAT3 gGridMax;
    float gCellSize; // h (uniform; equal to SPH support radius)

    XMFLOAT3 gGridDim; // cells per axis
    UINT gScanCount;
};
#endif
