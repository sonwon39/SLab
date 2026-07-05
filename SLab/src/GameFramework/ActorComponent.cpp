#include "ActorComponent.h"

ActorComponent::ActorComponent(Actor* owner) : m_owner(owner)
{
}

ActorComponent::~ActorComponent()
{
}

void ActorComponent::TickComponent(const float& deltaTime)
{
}
