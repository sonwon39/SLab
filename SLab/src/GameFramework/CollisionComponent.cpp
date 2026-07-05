#include "CollisionComponent.h"
#include "Actor.h"
#include "../StaticMesh.h"
#include "ActorData.h"

CollisionComponent::CollisionComponent(Actor* owner) : PrimitiveComponent(owner)
{
    m_mesh = std::make_shared<StaticMesh>();
}

CollisionComponent::~CollisionComponent()
{
}

void CollisionComponent::SetMesh(std::shared_ptr<StaticMesh> newMesh)
{
    m_mesh = std::move(newMesh);
}

void CollisionComponent::SetActorData(const ActorData& ad)
{
    SetPhysX(false);
    SetPhysXMode(PhysXMode::PM_Default);
    SetPSOName("collisionPSO");
    SetUpdateConstant(ad.updateConstants);

    SetLocation(ad.collisionLocation);
    // SetCollisionScale(ad.lc.collisionScale);
    // SetCollisionShape((PhysXShape)ad.lc.collisionShape);
}

// DirectX::SimpleMath::Vector3 CollisionComponent::GetCollisionScale() const
//{
//	return localConstant.collisionScale;
// }
