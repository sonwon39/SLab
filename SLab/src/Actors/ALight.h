#pragma once

#include "GameFramework/Actor.h"
#include "directxtk12/SimpleMath.h"

class ALight : public Actor
{
  public:
    ALight();
    virtual ~ALight();

  public:
    virtual void Initialize();

  public:
    virtual void Tick(const float& deltaTime);

  private:
    float m_velocity;
    DirectX::SimpleMath::Vector3 origin;
    DirectX::SimpleMath::Vector3 target;
    DirectX::SimpleMath::Vector3 dir;
    DirectX::SimpleMath::Vector3 originDir;
    float len = 0.;
};
