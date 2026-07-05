#include "Mover.h"
#include "GameFramework\Actor.h"


Mover::Mover(Actor* owner)
	: ActorComponent(owner)
{
}

Mover::~Mover()
{
}
void Mover::Initialize()
{
    ActorComponent::Initialize();
    SetComponentTickEnabled(true);
};

void Mover::TickComponent(const float& deltaTime)
{
    ActorComponent::TickComponent(deltaTime);

    Actor* owner = GetOwner();
	if (owner)
	{
        Vector3 currLoc = owner->GetActorLocation();
        Vector3 newLoc = currLoc + 1.f * deltaTime * Vector3(1.f, 0.f, 0.f);
        owner->SetActorLocation(newLoc);
	}
}
