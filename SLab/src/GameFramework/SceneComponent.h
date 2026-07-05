#pragma once

#include <vector>
#include <memory>
#include "d3d12.h"
#include "ActorComponent.h"
#include "directxtk12/SimpleMath.h"
#include "Transform.h"
#include "PBR/PBRHLSLCompat.h"
#include "PhysXMode.h"

class CameraComponent;
class CollisionComponent;

// 트랜스폼과 부모/자식 계층을 담당하는 컴포넌트.
// localTransform(부모 기준 로컬)이 위치/회전의 단일 원본이고, 부모의 월드 트랜스폼과 합쳐
// 이 컴포넌트의 월드 행렬(m_worldMatrix)을 만든다.
// 변경 시 자식들에게 월드 트랜스폼을 전파한다.
class SceneComponent : public ActorComponent
{
  public:
    SceneComponent(Actor* owner);
    virtual ~SceneComponent();

    void Attach(std::shared_ptr<SceneComponent> sceneComp);

  public:
    void SetSpeed(const float& newSpeed)
    {
        m_speed = newSpeed;
    }
    void SetRotateSpeed(const float& newSpeed)
    {
        m_rotateSpeed = newSpeed;
    }
    void UpdateWorldTransform(const Transform& tr);
    void UpdateRotation(const int& mouseDeltaX, const int& mouseDeltaY, const float& deltaTime);
    void UpdateRotation(const float& deltaAngleX, const float& deltaAngleY);

    void SetLocation(const DirectX::SimpleMath::Vector3& newLocation);
    void SetRotation(const DirectX::SimpleMath::Quaternion& newQuat);
    void SetLocalTransform(const Matrix& newMatrix);
    void SetLocalTransform(const Transform& newTransform);

    // physx에서 계산한 localTransform * worldTransform으로 부터 localTransform 추출
    void SetLocalTransformByLdotW(const Transform& LoW);
    void SetFrontDirection(const DirectX::SimpleMath::Vector3& newDir)
    {
        m_frontDirection = newDir;
    }
    void SetUpDirection(const DirectX::SimpleMath::Vector3& newDir)
    {
        m_upDirection = newDir;
    }
    void SetRightDirection(const DirectX::SimpleMath::Vector3& newDir)
    {
        m_rightDirection = newDir;
    }

    void AddLocation(const DirectX::SimpleMath::Vector3& delLocation);
    void AddRotation(const DirectX::SimpleMath::Quaternion& delQ);

  public:
    float GetSpeed() const
    {
        return m_speed;
    }
    float GetRotateSpeed() const
    {
        return m_rotateSpeed;
    }
    virtual DirectX::SimpleMath::Vector3 GetCollisionScale() const;
    DirectX::SimpleMath::Quaternion GetCollisionRotation() const;

    DirectX::SimpleMath::Vector3 GetFrontDirection() const
    {
        return m_frontDirection;
    }
    DirectX::SimpleMath::Vector3 GetBaseFrontDirection() const
    {
        return m_baseFrontDirection;
    }
    DirectX::SimpleMath::Vector3 GetBaseUpDirection() const
    {
        return m_baseUpDirection;
    }
    DirectX::SimpleMath::Vector3 GetUpDirection() const
    {
        return m_upDirection;
    }
    DirectX::SimpleMath::Vector3 GetRightDirection() const
    {
        return m_rightDirection;
    }

    DirectX::SimpleMath::Vector3 GetLocalLocation() const
    {
        return localTransform.location;
    }
    DirectX::SimpleMath::Quaternion GetLocalRotation() const
    {
        return localTransform.quat;
    }

    DirectX::SimpleMath::Vector3 GetLocation() const
    {
        return m_worldMatrix.Translation();
    }
    DirectX::SimpleMath::Vector3 GetCollisionLocation() const;
    DirectX::SimpleMath::Vector3 GetCollisionOffsetLocation() const;
    DirectX::SimpleMath::Quaternion GetCollisionOffsetRotation() const;
    DirectX::SimpleMath::Quaternion GetRotation() const;
    virtual DirectX::SimpleMath::Matrix GetViewMatrix() const;
    DirectX::SimpleMath::Matrix GetCameraViewMatrix() const;
    void GetChildrenComponents(std::vector<std::shared_ptr<SceneComponent>>& children) const;

    virtual DirectX::SimpleMath::Vector3 GetCameraLocation() const;
    virtual DirectX::SimpleMath::Matrix GetProjMatrix() const;

    virtual void UpdateConstantTransform();

    D3D12_GPU_VIRTUAL_ADDRESS GetGlboalConstant() const;
    CameraComponent* GetCameraComponent() const
    {
        return m_cameraComponent;
    }

  public:
    // world->actor 호출
    virtual void OnRegister();

  protected:
    Transform worldTransform; // 부모로부터 전파받은 월드 트랜스폼(= 부모의 월드)
    Transform localTransform; // 부모 기준 로컬 트랜스폼 (위치/회전의 단일 원본)
    // 이 컴포넌트의 월드 행렬(localTransform * worldTransform). 위치/회전 조회의 CPU 원본이며,
    // localConstant.model은 이걸 전치한 GPU 업로드용 파생값이다. (언리얼 ComponentToWorld 역할)
    DirectX::SimpleMath::Matrix m_worldMatrix;

    DirectX::SimpleMath::Vector3 m_baseUpDirection;
    DirectX::SimpleMath::Vector3 m_baseFrontDirection;
    DirectX::SimpleMath::Vector3 m_frontDirection;
    DirectX::SimpleMath::Vector3 m_upDirection;
    DirectX::SimpleMath::Vector3 m_rightDirection;

  protected:
    float m_speed = 5.f;
    float m_rotateSpeed = 60.f;
    float xAngle = 0.f;
    float yAngle = 0.f;

    float maxYAngle = 89.f;
    float minYAngle = -89.f;

  protected:
    SceneComponent* m_parent = nullptr;
    std::vector<std::shared_ptr<SceneComponent>> m_children;

    // 자식으로 부착된 카메라/콜리전을 Attach 시점에 1회만 캐싱한다(매 호출 dynamic_cast 제거).
    // 언리얼이 GetCapsuleComponent()처럼 서브오브젝트 포인터를 들고 있는 것과 같은 방식.
    CameraComponent* m_cameraComponent = nullptr;
    CollisionComponent* m_collisionComponent = nullptr;
};
