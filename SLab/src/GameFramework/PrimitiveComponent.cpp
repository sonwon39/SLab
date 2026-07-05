#include "PrimitiveComponent.h"
#include "Actor.h"
#include "Engine/World.h"
// #include "PhysXEngine.h"
#include "ActorData.h"

using DirectX::XMVECTOR;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;

PrimitiveComponent::PrimitiveComponent(Actor* owner) : SceneComponent(owner), m_visible(true)
{
}

PrimitiveComponent::~PrimitiveComponent()
{
}

std::string PrimitiveComponent::GetName() const
{
    return m_owner ? m_owner->GetName() : "";
}

physx::PxTransform PrimitiveComponent::GetPxTransform() const
{
    // TODO : quat 변경
    DirectX::SimpleMath::Vector3 loc = GetCollisionLocation();
    DirectX::SimpleMath::Quaternion q = GetCollisionRotation();
    return physx::PxTransform(physx::PxVec3(loc.x, loc.y, loc.z), physx::PxQuat(q.x, q.y, q.z, q.w));
}

void PrimitiveComponent::OnRegister()
{
    SceneComponent::OnRegister();
}

void PrimitiveComponent::SyncCB(float deltaTime)
{
    if (m_cbInitialized)
        m_cb.Update();
    if (m_mcbInitialized)
        m_materialCB.Update();
}

D3D12_GPU_VIRTUAL_ADDRESS PrimitiveComponent::GetCBGPUAddress() const
{
    return m_cbInitialized ? m_cb.GetGPUAddress() : 0;
}

D3D12_GPU_VIRTUAL_ADDRESS PrimitiveComponent::GetMCBGPUAddress() const
{
    return m_mcbInitialized ? m_materialCB.GetGPUAddress() : 0;
}

D3D12_GPU_VIRTUAL_ADDRESS PrimitiveComponent::GetSMCBGPUAddress() const
{
    return 0;
}

void PrimitiveComponent::SyncFromPhysX(const physx::PxTransform& transform)
{
    DirectX::SimpleMath::Vector3 loc(transform.p.x, transform.p.y, transform.p.z);
    DirectX::SimpleMath::Quaternion rot(transform.q.x, transform.q.y, transform.q.z, transform.q.w);

    Vector3 collLoc = GetCollisionOffsetLocation();
    Quaternion collRot = GetCollisionOffsetRotation();

    Matrix collMat = Matrix::CreateFromQuaternion(collRot) * Matrix::CreateTranslation(collLoc);

    Matrix ret = Matrix::CreateFromQuaternion(rot) * Matrix::CreateTranslation(loc);

    collMat = collMat.Invert();
    Matrix finalMat = collMat * ret;
    Transform t;
    finalMat.Decompose(t.scale, t.quat, t.location);

    SetLocalTransformByLdotW(t);
}

void PrimitiveComponent::SetActorData(const ActorData& ad)
{
    SetPhysX(ad.useSimulate);
    SetPhysXMode(ad.mode);
    SetPSOName(ad.psoName);
    SetUpdateConstant(ad.updateConstants);
    SetTextureName(ad.textureName);

    SetLocalConstant(ad.lc);

    if (ad.useMaterial)
        SetMaterialConstant(ad.mc);
}

void PrimitiveComponent::SetLocalConstant(const LocalConstant& newConstant)
{
    if (m_cbInitialized)
        m_cb.localConstant = newConstant;
    else
    {
        m_cb.Initialize(newConstant);
        m_cbInitialized = true;
    }
    // 입력 model은 GPU 업로드용 전치본이므로 한 번 더 전치해서 CPU 원본 transform을 복원.
    Matrix model = newConstant.model.Transpose();

    XMVECTOR s, q, t;
    DirectX::XMMatrixDecompose(&s, &q, &t, model);
    localTransform.location = t;
    localTransform.quat = q;
    localTransform.scale = s;

    UpdateConstantTransform();
}

void PrimitiveComponent::SetMaterialConstant(const MaterialConstant& newConstant)
{
    if (m_mcbInitialized)
        m_materialCB.localConstant = newConstant;
    else
    {
        m_materialCB.Initialize(newConstant);
        m_mcbInitialized = true;
    }
    m_materialCB.Update();
}

void PrimitiveComponent::UpdateConstantTransform()
{
    SceneComponent::UpdateConstantTransform(); // m_worldMatrix + 자식 전파

    // HLSL CB는 column-major를 기대하므로 model은 전치해서 보관하고,
    // modelInvTranspose는 전치 전 행렬의 역행렬(= 노멀 변환용)로 계산한다.
    m_cb.localConstant.modelInvTranspose = m_worldMatrix.Invert();
    m_cb.localConstant.model = m_worldMatrix.Transpose();
}
