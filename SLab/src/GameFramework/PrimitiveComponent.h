#pragma once
#include "SceneComponent.h"
#include "Core/ConstantBuffer.h"
#include "PBR/PBRHLSLCompat.h"
#include "d3d12.h"
#include <string>

#include "physx\PxPhysicsAPI.h"
#include "PhysXMode.h"

struct ActorData;

// 렌더 가능한 + 물리 표현을 갖는 SceneComponent. 가시성, PSO/텍스처 이름,
// PhysX 트랜스폼 동기화를 담당한다. (PhysX 연동은 추후 활성화 예정)
class PrimitiveComponent : public SceneComponent
{
  public:
    PrimitiveComponent(Actor* owner);
    virtual ~PrimitiveComponent();

  public:
    void SetVisible(bool visible)
    {
        m_visible = visible;
    }
    void SetPhysX(bool usePhysX)
    {
        m_usePhysX = usePhysX;
    }
    void SetPhysXMode(PhysXMode newMode)
    {
        m_physXMode = newMode;
    }
    void SetTextureName(const std::string& newName)
    {
        m_textureName = newName;
    }
    void SetPSOName(const std::string& newName)
    {
        m_psoName = newName;
    }

    virtual void SetActorData(const ActorData& ad);
    bool IsVisible() const
    {
        return m_visible;
    }
    bool IsKinematic() const
    {
        return m_physXMode == PhysXMode::PM_Kinematic;
    }

  public:
    // ComponentBeginOverlapSignature OnComponentBeginOverlap;
    // ComponentEndOverlapSignature OnComponentEndOverlap;

  public:
    std::string GetName() const;
    std::string GetTextureName() const
    {
        return m_textureName;
    };
    std::string GetPSOName() const
    {
        return m_psoName;
    };
    PhysXMode GetPhysXMode() const
    {
        return m_physXMode;
    }
    physx::PxTransform GetPxTransform() const;

    bool GetUpdateConstant() const
    {
        return m_updateConstant;
    }

  public:
    void OnRegister() override;

    // 물리 시뮬레이션 이후 transform 동기화
    void SyncFromPhysX(const physx::PxTransform& transform);
    void SetUpdateConstant(bool newState)
    {
        m_updateConstant = newState;
    }
    bool IsUpdateConstant()
    {
        return m_updateConstant;
    }

  public:
    // per-primitive CB 소유: 컴포넌트가 GPU 자원의 생명주기를 책임진다.
    // 매 프레임 MeshBatch::Tick 진입 시 SyncCB()로 GPU CB memcpy.
    virtual void SyncCB(float deltaTime);
    D3D12_GPU_VIRTUAL_ADDRESS GetCBGPUAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetMCBGPUAddress() const;
    virtual D3D12_GPU_VIRTUAL_ADDRESS GetSMCBGPUAddress() const;

  public:
    void SetLocalConstant(const LocalConstant& newConstant);
    LocalConstant GetLocalConstant() const
    {
        return m_cb.localConstant;
    }

    void SetMaterialConstant(const MaterialConstant& newConstant);
    MaterialConstant GetMaterialConstant() const
    {
        return m_materialCB.localConstant;
    }

    void UpdateConstantTransform() override;

  protected:
    bool m_visible;
    bool m_usePhysX = false;
    PhysXMode m_physXMode;
    bool m_updateConstant = false;
    std::string m_textureName;
    std::string m_psoName;

    ConstantBuffer<LocalConstant> m_cb;
    ConstantBuffer<MaterialConstant> m_materialCB;
    bool m_cbInitialized = false;
    bool m_mcbInitialized = false;
};
