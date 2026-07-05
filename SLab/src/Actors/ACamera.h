#pragma once

#include "GameFramework/Actor.h"
#include "directxtk12/SimpleMath.h"

class ACamera : public Actor
{
  public:
    ACamera();
    virtual ~ACamera();

  public:
    virtual void Initialize(DirectX::SimpleMath::Vector3& location, bool isPerspective);

  public:
    virtual void Tick(const float& deltaTime);

  private:
    float m_velocity;
};
