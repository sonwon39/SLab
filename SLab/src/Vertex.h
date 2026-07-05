#pragma once

#include "directxtk12/SimpleMath.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

struct PositionVertex
{
    Vector3 position;
};

struct SimpleVertex
{
    Vector3 position;
    Vector2 uv;
};

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
};

struct PBRVertex
{
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    Vector2 uv;
};

struct PointCloudVertex
{
    Vector3 position;
    Vector4 color;
};

struct SkinnedVertex
{
    DirectX::SimpleMath::Vector3 position;
    DirectX::SimpleMath::Vector3 normal;
    DirectX::SimpleMath::Vector3 tangent;
    DirectX::SimpleMath::Vector2 texcoord;

    float blendWeights[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // BLENDWEIGHT0 and 1
    uint8_t boneIndices[8] = {0, 0, 0, 0, 0, 0, 0, 0};                        // BLENDINDICES0 and 1
};