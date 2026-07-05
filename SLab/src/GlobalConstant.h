#ifndef GLOBALCONSTNAT_H
#define GLOBALCONSTNAT_H

#ifdef HLSL
#include "HlslCompat.h"
#else
using namespace DirectX::SimpleMath;
using namespace DirectX;
#endif

#define NUM_LIGHTS 3
#define MAX_LIGHT 15

struct Light
{
    int enabled;
    XMFLOAT3 position;
    
    XMFLOAT3 direction;
    float fallOffEnd;

    XMFLOAT3 radiance;
    float dummy2;

#ifndef HLSL
    Light() : enabled(0), position(0.f, 0.0f, 0.0f), radiance(1.f,1.f,1.f)
    {
    }
#endif
};

struct GlobalConstant
{
    Matrix view;
    Matrix projection;

    XMFLOAT3 cameraPos;
};

struct LightConstant
{
    Light lights[NUM_LIGHTS];
};

struct MouseConstant
{
    UINT posX;
    UINT posY;
    XMFLOAT2 velocity;

    bool lButtonDown;
    XMFLOAT3 color;
};

#endif
