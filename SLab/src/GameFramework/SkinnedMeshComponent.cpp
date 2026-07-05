#include "SkinnedMeshComponent.h"
#include "Actor.h"
#include "GraphicsCommon.h"
#include "StaticMesh.h"
#include "MeshBatch.h"
#include "Engine\RenderEngine.h"
#include "Engine\World.h"
#include "AssetManager\ModelLoader.h"

SkinnedMeshComponent::SkinnedMeshComponent(Actor* owner) : PrimitiveComponent(owner)
{
    m_mesh = std::make_shared<StaticMesh>();
}

SkinnedMeshComponent::~SkinnedMeshComponent()
{
}


void SkinnedMeshComponent::SyncCB(float deltaTime)
{
    if (m_scbInitialized)
    {
        UpdateAnimation(deltaTime);
	}
}

void SkinnedMeshComponent::SetMesh(std::shared_ptr<StaticMesh> newMesh)
{
    m_mesh = std::move(newMesh);
}

D3D12_GPU_VIRTUAL_ADDRESS SkinnedMeshComponent::GetSMCBGPUAddress() const
{
    return m_scbInitialized ? m_skinnedLCB.GetGPUAddress() : 0;
}

void SkinnedMeshComponent::OnRegister()
{
    PrimitiveComponent::OnRegister(); // per-primitive CB 초기화

    auto mb = std::make_shared<MeshBatch>();
    mb->mesh = m_mesh.get();
    mb->material = Graphics::m_world ? Graphics::m_world->GetOrCreateMaterial(m_textureName).get() : nullptr;
    mb->owner = this;
    mb->psoName = m_psoName;

    m_renderEngine->RegistMeshBatch(mb);
}

void SkinnedMeshComponent::UpdateAnimation(const float& deltaTime)
{
	using namespace Graphics;

    if (bUpdateAnim && m_world && m_owner)
    {
        ActorState as = m_owner->GetActorState();
        
         m_currentFrame += m_animationSpeed * deltaTime;
         auto modelLoader = m_world->GetSkinnedModelLoader();
         modelLoader->UpdateSLC(m_currentFrame, m_animationName, 0, bUpdateRoot,m_skinnedLCB);
    }
    for (const auto& c : m_children)
    {
        if (SkinnedMeshComponent* comp = dynamic_cast<SkinnedMeshComponent*>(c.get()))
        {
            comp->UpdateAnimation(deltaTime);
        }
    }
}

void SkinnedMeshComponent::PlayMontage(const float& deltaTime)
{
    m_currentFrame += m_animationSpeed * deltaTime;
    if (m_animationSize - m_currentFrame < 4.f)
    {
        m_currentBlendData.frame0 = m_currentFrame;
        m_currentBlendData.frame1 = 0;
        m_currentBlendData.animName0 = m_animationName;
        m_currentBlendData.animName1 = m_animationNames[ActorState::AS_idle];
        m_currentBlendData.weight = 1.f - ((m_animationSize - m_currentFrame) / 4.f);
        m_currentBlendData.clipId0 = 0;
        m_currentBlendData.clipId1 = 0;
        bBlendPose = true;
    }
    if (m_currentFrame > m_animationSize)
    {
        m_animationName = m_animationNames[ActorState::AS_idle];
        m_owner->SetActorState(ActorState::AS_idle);
        m_currentFrame = 0.f;
        bBlendPose = false;
    }
}

void SkinnedMeshComponent::SetAnimationData(const AnimData& animData)
{
    SetAnimationSpeed(animData.animationSpeed);
    std::string idle = animData.name + "_Idle";
    std::string attack = animData.name + "_AttackD";

    SetAnimationName(ActorState::AS_default, animData.name);
    SetAnimationName(ActorState::AS_idle, idle);
    SetAnimationName(ActorState::AS_attack, attack);
    m_animationName = m_animationNames[animData.actorState];

    SetPlayAnimation(animData.playAnimation);

	if (!m_scbInitialized)
	{
        m_scbInitialized = true;
        m_skinnedLCB.Initialize({});
        m_skinnedLCB.Update();
	}
}
