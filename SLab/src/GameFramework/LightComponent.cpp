#include "LightComponent.h"
#include"GraphicsCommon.h"
#include "Engine\World.h"

using namespace Graphics;

LightComponent::LightComponent(Actor* owner) : SceneComponent(owner)
{
}

LightComponent::~LightComponent()
{
}

void LightComponent::OnRegister()
{
    SceneComponent::OnRegister();

	if (!m_world)
        return;
    lightManager = m_world->GetLightManger();
    m_slot = lightManager->Allocate();
    UpdateLightConstant(tempConstant);
}

void LightComponent::Initialize(Light light)
{
    tempConstant = light;
}

void LightComponent::UpdateLightConstant(Light light)
{
    if (m_slot != -1 && lightManager)
    {
        auto& l = lightManager->At(m_slot);
        l = light;
    }
}

void LightComponent::UpdateConstantTransform()
{
    SceneComponent::UpdateConstantTransform();

	if (m_slot != -1 && lightManager)
    {
        auto& l = lightManager->At(m_slot);
        l.position = GetLocation();
    }
}
