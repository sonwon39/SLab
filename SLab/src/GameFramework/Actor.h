#pragma once

#include <memory>
#include "d3d12.h"
#include "directxtk12\SimpleMath.h"

#include "SceneComponent.h"
#include "ActorData.h"

class World;
class StaticMesh;
class PrimitiveComponent;
class SkinnedMeshComponent;
class CameraComponent;

// 씬에 배치되는 오브젝트의 기본 단위(언리얼 Actor 모델). 자체 트랜스폼은 갖지 않고
// 루트 SceneComponent에 위임하며, 대부분의 메서드는 m_rootComponent로의 포워딩이다.
class Actor
{
  public:
    Actor();
    Actor(std::string actorName);

  public:
    virtual void Initialize(std::shared_ptr<StaticMesh> mesh, const ActorData& ad);

  public:
    virtual void Tick(const float& deltaTime);
    void TickComponents(const float& deltaTime);
    
  public:
    void UpdateRotation(const int& mouseDeltaX, const int& mouseDeltaY, const float& deltaTime);
    void UpdateCameraInfo(const int& width, const int& height);
    void SetActorLocation(const DirectX::SimpleMath::Vector3& newLocation);
    void SetActorRotation(const DirectX::SimpleMath::Quaternion& newQuat);
    void UpdateActorLocation(const DirectX::SimpleMath::Vector3& delLocation);
    void UpdateActorRotation(const DirectX::SimpleMath::Quaternion& delQuat);
    void SetActorSpeed(const float& newSpeed);
    void SetRootComponent(std::shared_ptr<SceneComponent> newRootComponent);
    void AddActorComponent(std::shared_ptr<ActorComponent> component);

  public:
    void OnRegister();

  public:
    SceneComponent* GetRootComponent() const
    {
        return m_rootComponent.get();
    }
    std::string GetName() const
    {
        return m_name;
    }

  public:
    DirectX::SimpleMath::Vector3 GetActorLocation() const;
    DirectX::SimpleMath::Vector3 GetActorFrontDir() const;
    DirectX::SimpleMath::Vector3 GetCameraLocation() const;
    DirectX::SimpleMath::Matrix GetProjMatrix() const;
    DirectX::SimpleMath::Vector3 GetActorUpDir() const;
    DirectX::SimpleMath::Vector3 GetActorRightDir() const;
    float GetActorSpeed() const;
    DirectX::SimpleMath::Matrix GetViewMatrix() const;
    DirectX::SimpleMath::Matrix GetCameraViewMatrix() const;
    CameraComponent* GetCameraComponent() const;
    ActorState GetActorState() const
    {
        return m_currentState;
    }

  public:
    D3D12_GPU_VIRTUAL_ADDRESS GetGlboalConstant() const;

  public:
    void SetUpdateConstant(bool newState);
    void UpdateAnimation(float deltaTime);
    void SetLocalConstant(const LocalConstant& newLocalConstant);
    void SetTextureName(const std::string& newName);
    void SetPSOName(const std::string& newName);
    void SetActorState(const ActorState newState)
    {
        m_currentState = newState;
    }

    void SetMaterialConstant(const MaterialConstant& newConstant);
    MaterialConstant GetMaterialConstant() const;

  protected:
    std::shared_ptr<SceneComponent> m_rootComponent;
    // 루트의 구체 타입 캐시. SetRootComponent에서 1회 산출 (각 Setter의 반복 dynamic_cast 제거)
    PrimitiveComponent* m_rootPrimitive = nullptr;
    SkinnedMeshComponent* m_rootSkinned = nullptr;
    std::string m_name;

  protected:
    std::vector<std::shared_ptr<ActorComponent>> m_ownedComponents;

  private:
    ActorState m_currentState;
};
