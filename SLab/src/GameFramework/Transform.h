#pragma once

#include "directxtk12\SimpleMath.h"

struct Transform
{

    Transform() {};
    Transform(DirectX::SimpleMath::Matrix m)
    {
        m.Decompose(scale, quat, location);
    }
    DirectX::SimpleMath::Vector3 location;
    DirectX::SimpleMath::Quaternion quat;
    DirectX::SimpleMath::Vector3 scale = DirectX::SimpleMath::Vector3(1.f, 1.f, 1.f);

    // SRT 순서(scale → rotation → translation)로 합성한 모델 행렬. 행벡터 규약(row-major)이라 곱 순서가 S*R*T.
    DirectX::SimpleMath::Matrix ToMatrix() const
    {
        return DirectX::SimpleMath::Matrix::CreateScale(scale) *
               DirectX::SimpleMath::Matrix::CreateFromQuaternion(quat) *
               DirectX::SimpleMath::Matrix::CreateTranslation(location);
    }
};
