#pragma once

#include "GameFramework/Actor.h"
#include "directxtk12/SimpleMath.h"

class AMovingPlatform : public Actor
{
  public:
    AMovingPlatform();
    virtual ~AMovingPlatform();

  public:
    virtual void Initialize();

  public:
    virtual void Tick(const float& deltaTime);

  private:
    // DirectX::SimpleMath::Vector3 m_velocity;
    float m_velocity;
};
