#include "SceneComponent.h"
#include "directxtk12/SimpleMath.h"
#include "StaticMeshComponent.h"
#include "CollisionComponent.h"
#include "CameraComponent.h"

using DirectX::SimpleMath::Vector3;

SceneComponent::SceneComponent(Actor* owner)
    : ActorComponent(owner), m_frontDirection(Vector3(0, 0, 1)), m_baseFrontDirection(Vector3(0, 0, 1)),
      m_upDirection(Vector3(0, 1, 0)), m_baseUpDirection(Vector3(0, 1, 0)), m_rightDirection(Vector3(1, 0, 0))
{
}

SceneComponent::~SceneComponent()
{
}

void SceneComponent::Attach(std::shared_ptr<SceneComponent> sceneComp)
{
    sceneComp->m_parent = this;
    m_children.push_back(sceneComp);

    // 부착 시점에 한 번만 타입을 확인해 캐싱한다(이후 조회는 dynamic_cast 없이 포인터 직접 사용).
    if (auto* cam = dynamic_cast<CameraComponent*>(sceneComp.get()))
        m_cameraComponent = cam;
    if (auto* col = dynamic_cast<CollisionComponent*>(sceneComp.get()))
        m_collisionComponent = col;

    UpdateConstantTransform();
}

void SceneComponent::UpdateWorldTransform(const Transform& tr)
{
    worldTransform = tr;
    UpdateConstantTransform();
}

void SceneComponent::UpdateRotation(const int& mouseDeltaX, const int& mouseDeltaY, const float& deltaTime)
{
    float delX = mouseDeltaX * m_rotateSpeed * deltaTime;
    float delY = mouseDeltaY * m_rotateSpeed * deltaTime;
    xAngle += delX;
    if (xAngle >= 360)
    {
        xAngle -= 360;
    }
    if (xAngle <= -360)
    {
        xAngle += 360;
    }

    float xRadian = DirectX::XMConvertToRadians(xAngle);

    DirectX::SimpleMath::Matrix m_rotation = DirectX::XMMatrixRotationY(xRadian);
    DirectX::SimpleMath::Quaternion yRotQ = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(m_rotation);

    DirectX::SimpleMath::Vector3 m_frontDir =
        DirectX::SimpleMath::Vector3::Transform(GetBaseFrontDirection(), m_rotation);
    DirectX::SimpleMath::Vector3 m_rightDir = GetBaseUpDirection().Cross(m_frontDir);
    ;

    if (yAngle + delY >= maxYAngle)
        delY = maxYAngle - yAngle;
    else if (yAngle + delY <= minYAngle)
        delY = minYAngle - yAngle;

    yAngle += delY;
    // std::cout << yAngle << '\n';
    float yRadian = DirectX::XMConvertToRadians(yAngle);

    m_rotation = DirectX::XMMatrixRotationAxis(m_rightDir, yRadian);
    m_frontDir = DirectX::SimpleMath::Vector3::Transform(m_frontDir, m_rotation);

    SetFrontDirection(m_frontDir);
    SetRightDirection(m_rightDir);
    SetRotation(yRotQ);
}

void SceneComponent::UpdateRotation(const float& deltaAngleX, const float& deltaAngleY)
{
    float delX = deltaAngleX;
    float delY = deltaAngleY;
    xAngle += delX;
    if (xAngle >= 360)
    {
        xAngle -= 360;
    }
    if (xAngle <= -360)
    {
        xAngle += 360;
    }

    float xRadian = DirectX::XMConvertToRadians(xAngle);

    DirectX::SimpleMath::Matrix m_rotation = DirectX::XMMatrixRotationY(xRadian);
    DirectX::SimpleMath::Quaternion yRotQ = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(m_rotation);

    DirectX::SimpleMath::Vector3 m_frontDir =
        DirectX::SimpleMath::Vector3::Transform(GetBaseFrontDirection(), m_rotation);
    DirectX::SimpleMath::Vector3 m_rightDir = GetBaseUpDirection().Cross(m_frontDir);
    ;

    if (yAngle + delY >= maxYAngle)
        delY = maxYAngle - yAngle;
    else if (yAngle + delY <= minYAngle)
        delY = minYAngle - yAngle;

    yAngle += delY;
    // std::cout << yAngle << '\n';
    float yRadian = DirectX::XMConvertToRadians(yAngle);

    m_rotation = DirectX::XMMatrixRotationAxis(m_rightDir, yRadian);
    m_frontDir = DirectX::SimpleMath::Vector3::Transform(m_frontDir, m_rotation);

    SetFrontDirection(m_frontDir);
    SetRightDirection(m_rightDir);
    SetRotation(yRotQ);
}
void SceneComponent::SetLocation(const DirectX::SimpleMath::Vector3& newLocation)
{
    localTransform.location = newLocation;
    UpdateConstantTransform();
}

void SceneComponent::SetRotation(const DirectX::SimpleMath::Quaternion& newQuat)
{
    // TODO front direction도 회전 시켜야 함
    localTransform.quat = newQuat;
    UpdateConstantTransform();
}
void SceneComponent::SetLocalTransform(const Matrix& newMatrix)
{
    // TODO front direction도 회전 시켜야 함
    Transform t(newMatrix);
    localTransform = t;
    UpdateConstantTransform();
}
void SceneComponent::SetLocalTransform(const Transform& newTransform)
{
    // TODO front direction도 회전 시켜야 함
    localTransform = newTransform;
    UpdateConstantTransform();
}
void SceneComponent::SetLocalTransformByLdotW(const Transform& LoW)
{
    Matrix m = LoW.ToMatrix() * worldTransform.ToMatrix().Invert();
    Transform t(m);

    localTransform = m;
    UpdateConstantTransform();
}
void SceneComponent::AddLocation(const DirectX::SimpleMath::Vector3& delLocation)
{
    localTransform.location += delLocation;
    UpdateConstantTransform(); // 자식 전파까지 여기서 처리됨
}

void SceneComponent::AddRotation(const DirectX::SimpleMath::Quaternion& delQ)
{
    localTransform.quat *= delQ;
    UpdateConstantTransform();
}

// 콜리전 조회: 캐싱된 콜리전 컴포넌트가 있으면 그쪽 값.
// SceneComponent 자체는 머티리얼 데이터를 들지 않으므로, 기본은 identity scale.
// PrimitiveComponent에서 자신의 localConstant.collisionScale 폴백을 override한다.
DirectX::SimpleMath::Vector3 SceneComponent::GetCollisionScale() const
{
    if (m_collisionComponent)
        return m_collisionComponent->GetCollisionScale();
    return DirectX::SimpleMath::Vector3(1.f, 1.f, 1.f);
}

DirectX::SimpleMath::Quaternion SceneComponent::GetCollisionRotation() const
{
    return m_collisionComponent ? m_collisionComponent->GetRotation() : GetRotation();
}

DirectX::SimpleMath::Vector3 SceneComponent::GetCollisionLocation() const
{
    return m_collisionComponent ? m_collisionComponent->GetLocation() : GetLocation();
}

DirectX::SimpleMath::Vector3 SceneComponent::GetCollisionOffsetLocation() const
{
    return m_collisionComponent ? m_collisionComponent->GetLocalLocation() : GetLocalLocation();
}

DirectX::SimpleMath::Quaternion SceneComponent::GetCollisionOffsetRotation() const
{
    return m_collisionComponent ? m_collisionComponent->GetLocalRotation() : GetLocalRotation();
}

DirectX::SimpleMath::Quaternion SceneComponent::GetRotation() const
{
    // 월드 회전. localConstant.model(전치된 GPU용)이 아니라 CPU 원본 m_worldMatrix에서 분해한다.
    // Decompose는 비const 멤버 함수라 지역 복사본에서 호출한다.
    Matrix m = m_worldMatrix;
    Vector3 scale;
    Quaternion rot;
    Vector3 translation;
    m.Decompose(scale, rot, translation);
    return rot;
}

DirectX::SimpleMath::Matrix SceneComponent::GetViewMatrix() const
{
    if (m_parent)
    {
        return XMMatrixLookToLH(GetLocation(), m_parent->m_frontDirection, m_parent->m_upDirection);
    }
    return XMMatrixLookToLH(GetLocation(), m_frontDirection, m_upDirection);
}

DirectX::SimpleMath::Matrix SceneComponent::GetCameraViewMatrix() const
{
    if (m_cameraComponent)
        return m_cameraComponent->GetViewMatrix();
    return XMMatrixLookToLH(GetLocation(), m_frontDirection, m_upDirection);
}

void SceneComponent::GetChildrenComponents(std::vector<std::shared_ptr<SceneComponent>>& children) const
{
    children = m_children;
}

void SceneComponent::OnRegister()
{
    for (const auto& c : m_children)
    {
        c->OnRegister();
    }
}

DirectX::SimpleMath::Vector3 SceneComponent::GetCameraLocation() const
{
    return m_cameraComponent ? m_cameraComponent->GetLocation() : Vector3(0.f, 0.f, 0.f);
}

DirectX::SimpleMath::Matrix SceneComponent::GetProjMatrix() const
{
    return m_cameraComponent ? m_cameraComponent->GetProjMatrix() : DirectX::SimpleMath::Matrix();
}

void SceneComponent::UpdateConstantTransform()
{
    // 로컬 * 부모월드 = 이 컴포넌트의 월드 행렬. 위치/회전 조회의 CPU 원본이다.
    // localConstant.model / modelInvTranspose 갱신은 PrimitiveComponent::UpdateConstantTransform이 담당.
    m_worldMatrix = localTransform.ToMatrix() * worldTransform.ToMatrix();

    for (const auto& c : m_children)
    {
        Transform t(m_worldMatrix);
        c->UpdateWorldTransform(t);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS SceneComponent::GetGlboalConstant() const
{
    return m_cameraComponent ? m_cameraComponent->GetGCBGPUAddress() : 0;
}
