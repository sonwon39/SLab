#ifndef DEFUALTHLSLCOMPAT_H
#define DEFUALTHLSLCOMPAT_H

#ifdef HLSL
#include "HlslCompat.h"
#else
using namespace DirectX::SimpleMath;
using namespace DirectX;
#endif

struct DefaultLocalConstant
{
    Matrix model;
    Matrix view;
    Matrix projection;
};

#endif
