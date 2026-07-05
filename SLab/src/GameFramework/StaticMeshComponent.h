#pragma once

#include "PrimitiveComponent.h"
#include "ActorData.h"

class StaticMesh;

class StaticMeshComponent : public PrimitiveComponent
{
  public:
    StaticMeshComponent(Actor* owner);
    virtual ~StaticMeshComponent();

  public:
    void SetMesh(std::shared_ptr<StaticMesh> newMesh);
    void OnRegister() override;

  public:
    StaticMesh* GetMesh() const
    {
        return m_mesh.get();
    }
    std::shared_ptr<StaticMesh> GetMeshPtr() const
    {
        return m_mesh;
    }

  private:
    std::shared_ptr<StaticMesh> m_mesh;
};
