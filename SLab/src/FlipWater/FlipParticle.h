#ifndef FLIP_PARTICLE_H
#define FLIP_PARTICLE_H

// Shared C++/HLSL header for the FLIP water module (independent from SPH).
// HLSL side: #define HLSL then include -> HlslCompat maps XMFLOAT3->float3 etc.
// C++  side: DirectX::SimpleMath types.
#ifdef HLSL
#include "../HlslCompat.h"
#else
using namespace DirectX::SimpleMath;
using namespace DirectX;
#endif

#define FLIP_GROUP_SIZE 256

// A FLIP particle. Step 1 (gravity) uses position + velocity only.
// radius is for the point renderer. 32 bytes, 16-byte aligned.
struct FlipParticle
{
    XMFLOAT3 position;
    float radius;
    XMFLOAT3 velocity;
    float pad;
    XMFLOAT3 color;
    float pad2;
};

// Simulation constants (compute cbuffer b0). Hand-packed to 16-byte rows.
struct FlipConstant
{
    UINT particleCount;
    float dt;
    float pad0;
    float pad1;

    XMFLOAT3 gravity;
    float pad2;

    XMFLOAT3 boundsMin;
    float pad3;

    XMFLOAT3 boundsMax;
    float pad4;
};

// Per-frame render constant for the SSF pipeline (billboard/depth/smooth/composite/bg).
// Matches FlipWater's FrameCB. Matrices stored TRANSPOSED (HLSL mul(v,M) convention).
struct FlipFrameConstant
{
    Matrix view;         // world->view (stored transposed)
    Matrix proj;         // view->clip  (stored transposed)
    XMFLOAT4 camPos;     // xyz = camera world pos
    XMFLOAT4 camRight;   // world-space camera basis
    XMFLOAT4 camUp;
    XMFLOAT4 camFwd;     // linear view depth = dot(hp-ro, camFwd)
    XMFLOAT4 sunDir;     // xyz = toward sun
    XMFLOAT4 projA;      // (P00, P11, P33, P43) of the actual proj matrix
    XMFLOAT4 screen;     // (W, H, 1/W, 1/H)
    XMFLOAT4 fluidA;     // (renderRadius, thicknessScale, zNear, zFar)
    XMFLOAT4 fluidB;     // (filterWorldRadius, depthThresh, maxFilterPx, 0)
    XMFLOAT4 absorb;     // (Beer-Lambert sigma.rgb, refractStrength)
    Matrix invViewProj;  // inverse(view*proj) for background ray reconstruction
};

#endif // FLIP_PARTICLE_H
