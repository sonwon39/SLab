#pragma once

#include "ConstantBuffer.h"
#include "GraphicsCommon.h"

template <typename Constant> inline void ConstantBuffer<Constant>::Initialize(const Constant& init)
{
    localConstant = init;
    Graphics::utility->CreateConstantBuffer(sizeof(Constant), m_localCB, &pLocalCB);
    memcpy(pLocalCB, &localConstant, sizeof(Constant));
}

template <typename Constant> inline void ConstantBuffer<Constant>::Update()
{
    memcpy(pLocalCB, &localConstant, sizeof(Constant));
}
