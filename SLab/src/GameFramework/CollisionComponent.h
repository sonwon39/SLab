#pragma once

#include "PrimitiveComponent.h"

class StaticMesh;

class CollisionComponent : public PrimitiveComponent
{
  public:
    CollisionComponent(Actor* owner);
    virtual ~CollisionComponent();

  public:
    void SetMesh(std::shared_ptr<StaticMesh> newMesh);

  public:
    StaticMesh* GetMesh() const
    {
        return m_mesh.get();
    }
    std::shared_ptr<StaticMesh> GetMeshPtr() const
    {
        return m_mesh;
    }
    void SetActorData(const ActorData& ad) override;
    // DirectX::SimpleMath::Vector3 GetCollisionScale() const override;

  private:
    std::shared_ptr<StaticMesh> m_mesh;
};
