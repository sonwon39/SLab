#ifndef PBRHLSLCOMPAT_H
#define PBRHLSLCOMPAT_H

#ifdef HLSL
#include "../HlslCompat.h"
#else
using namespace DirectX::SimpleMath;
using namespace DirectX;
#endif


struct MaterialConstant
{
    Matrix texTransform;

    float heightScale;
    float roughness;
    float metallic;
    float dummy;

	int useHeightMap;
    int useNormalMap;
    int useMetallicMap;
    int useRoughnessMap;
};

struct LocalConstant
{
    Matrix model;
    Matrix modelInvTranspose;
};

struct SkinnedLocalConstant
{
    Matrix boneTransform[300];
};

#endif
