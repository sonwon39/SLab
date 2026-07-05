#pragma once

#include "SceneComponent.h"
#include "d3d12.h"
#include <string>
#include <memory>

#include "AssetManager\LightManager.h"
#include "GlobalConstant.h"

struct ActorData;

class LightComponent : public SceneComponent
{
  public:
    LightComponent(Actor* owner);
    virtual ~LightComponent();

  public:
    void OnRegister() override;

	void Initialize(Light light);
    void UpdateLightConstant(Light light);
  protected:
    virtual void UpdateConstantTransform();

  protected:
    std::shared_ptr<LightManager> lightManager;
    Light tempConstant;

  public:
    int m_slot = -1;
};
