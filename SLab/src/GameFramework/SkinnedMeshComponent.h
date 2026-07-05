#pragma once

#include "PrimitiveComponent.h"
#include <directxtk12\SimpleMath.h>
#include "ActorData.h"
#include "AssetManager\BlendData.h"
#include "Core/ConstantBuffer.h"

class StaticMesh;
enum ActorState;

class SkinnedMeshComponent : public PrimitiveComponent
{
  public:
    SkinnedMeshComponent(Actor* owner);
    virtual ~SkinnedMeshComponent();

  public:
    void SetMesh(std::shared_ptr<StaticMesh> newMesh);
    void OnRegister() override;
    void SyncCB(float deltaTime) override;

  public:
    StaticMesh* GetMesh() const
    {
        return m_mesh.get();
    }
    std::string GetAnimationName() const
    {
        return m_animationName;
    }
    std::shared_ptr<StaticMesh> GetMeshPtr() const
    {
        return m_mesh;
    }
    SkinnedLocalConstant GetSkinnedLocalConstant()
    {
        return m_skinnedLCB.localConstant;
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetSMCBGPUAddress() const override;

  public:
    void SetAnimationSpeed(const float& newSpeed)
    {
        m_animationSpeed = newSpeed;
    }
    void SetAnimationName(const std::string& newName)
    {
        m_animationName = newName;
    }
    void SetAnimationName(const ActorState& as, const std::string& animName)
    {
        m_animationNames[as] = animName;
    }
    void UpdateAnimation(const float& deltaTime);
    void PlayMontage(const float& deltaTime);
    void SetAnimationData(const AnimData& animData);
    void SetPlayAnimation(const bool& playAnimation)
    {
        bUpdateAnim = playAnimation;
    }

  private:
    bool bUpdateAnim = true;
    bool bUpdateRoot = true;
    bool bBlendPose = false;
    float m_currentFrame = 0.f;
    float m_animationSpeed = 60.f;

    std::shared_ptr<StaticMesh> m_mesh;
    SkinnedLocalConstant m_skinnedLocalConstant;
    ConstantBuffer<SkinnedLocalConstant> m_skinnedLCB;
    std::string m_animationName;
    std::unordered_map<ActorState, std::string> m_animationNames;
    size_t m_animationSize;
    bool m_scbInitialized = false;

  private:
    struct BlendData m_currentBlendData;
};
