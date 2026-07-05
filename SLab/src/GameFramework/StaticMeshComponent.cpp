#include "StaticMeshComponent.h"
#include "Actor.h"
#include "StaticMesh.h"
#include "MeshBatch.h"
#include "Material.h"
#include "Engine/RenderEngine.h"
#include "Engine/World.h"
#include "GraphicsCommon.h"

using namespace Graphics;

StaticMeshComponent::StaticMeshComponent(Actor* owner) : PrimitiveComponent(owner)
{
    m_mesh = std::shared_ptr<StaticMesh>();
}

StaticMeshComponent::~StaticMeshComponent()
{
}

void StaticMeshComponent::SetMesh(std::shared_ptr<StaticMesh> newMesh)
{
    m_mesh = std::move(newMesh);
}

void StaticMeshComponent::OnRegister()
{
    PrimitiveComponent::OnRegister(); // per-primitive CB 초기화

    auto mb = std::make_shared<MeshBatch>();
    mb->mesh = m_mesh.get();
    mb->material = m_world ? m_world->GetOrCreateMaterial(m_textureName).get() : nullptr;
    mb->owner = this;
    mb->psoName = m_psoName;

    m_renderEngine->RegistMeshBatch(mb);
}
