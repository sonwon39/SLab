#include "CameraComponent.h"
#include "Engine/World.h"
#include "Engine/RenderEngine.h"
#include "GraphicsCommon.h"

CameraComponent::CameraComponent(Actor* owner) : SceneComponent(owner)
{
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::Initialize(const float& fovDegrees, const UINT& width, const UINT& height, const float& nearZ,
                                 const float& farZ, bool isPerspective)
{
    m_perspectiveMode = isPerspective;
    m_width = width;
    m_height = height;
    m_fovRadians = DirectX::XMConvertToRadians(fovDegrees);
    m_aspectRatio = (float)width / height;
    m_nearZ = nearZ;
    m_farZ = farZ;

    Matrix view;
    Matrix projection = CreateProjMatrix();

    auto loc = GetLocation();
    if (m_parent)
        view = XMMatrixLookToLH(loc, m_parent->GetFrontDirection(), m_parent->GetUpDirection());
    else
        view = XMMatrixLookToLH(loc, m_frontDirection, m_upDirection);

    view = view.Transpose();
    projection = projection.Transpose();

    if (!m_gcbInitialized)
    {
        m_gcbInitialized = true;

        GlobalConstant gc;
        gc.projection = projection;
        gc.view = view;

        m_gcb.Initialize(gc);
    }
    else
    {
        m_gcb.localConstant.projection = projection;
        m_gcb.localConstant.view = view;
    }
    m_gcb.localConstant.cameraPos = loc;
    m_gcb.Update();
}

void CameraComponent::Bind(const RootSignature* rs, ID3D12GraphicsCommandList* cl) const
{
    if (!rs || !cl)
        return;

    int gcb = rs->GetSlot(BindKey::GlobalCB);
    if (gcb >= 0)
        cl->SetGraphicsRootConstantBufferView(gcb, GetGCBGPUAddress());
}

void CameraComponent::UpdateCameraInfo(const int& width, const int& height)
{
    if (!m_gcbInitialized)
        return;
    m_width = width;
    m_height = height;
    m_aspectRatio = float(width) / height;

    Matrix projection = CreateProjMatrix();

    m_gcb.localConstant.projection = projection.Transpose();
}

void CameraComponent::SyncCB()
{
    if (m_gcbInitialized)
        m_gcb.Update();
}

void CameraComponent::OnRegister()
{
    Graphics::m_renderEngine->RegistCamera(this);
}

D3D12_GPU_VIRTUAL_ADDRESS CameraComponent::GetGCBGPUAddress() const
{
    return m_gcbInitialized ? m_gcb.GetGPUAddress() : 0;
}

void CameraComponent::UpdateConstantTransform()
{
    SceneComponent::UpdateConstantTransform();
    auto loc = GetLocation();
    Matrix view;
    if (m_parent)
        view = XMMatrixLookToLH(loc, m_parent->GetFrontDirection(), m_parent->GetUpDirection());
    else
        view = XMMatrixLookToLH(loc, m_frontDirection, m_upDirection);

    m_gcb.localConstant.cameraPos = loc;
    m_gcb.localConstant.view = view.Transpose();
}

DirectX::SimpleMath::Matrix CameraComponent::GetProjMatrix() const
{
    if (!m_gcbInitialized)
        return DirectX::SimpleMath::Matrix();
    return m_gcb.localConstant.projection;
}

DirectX::SimpleMath::Matrix CameraComponent::GetViewMatrix() const
{
    if (!m_gcbInitialized)
        return DirectX::SimpleMath::Matrix();
    return m_gcb.localConstant.view;
}

Matrix CameraComponent::CreateProjMatrix() const
{
	if (m_perspectiveMode)
	{
        return DirectX::XMMatrixPerspectiveFovLH(m_fovRadians, m_aspectRatio, m_nearZ, m_farZ);
	}
	else
	{
        return DirectX::XMMatrixOrthographicLH(2.f, 2.f, m_nearZ, m_farZ);
	}
}
