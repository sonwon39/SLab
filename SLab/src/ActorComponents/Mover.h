#pragma once

#include "GameFramework\ActorComponent.h"

class Mover : public ActorComponent
{
  public:
    Mover(Actor* owner);
    virtual ~Mover();

	virtual void Initialize() override;

  public:
	virtual void TickComponent(const float& deltaTime) override;


};
