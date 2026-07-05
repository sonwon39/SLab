#pragma once

#include "SceneComponent.h"
#include "d3d12.h"
#include "directxtk12\SimpleMath.h"
#include "GlobalConstant.h"
#include "Core/ConstantBuffer.h"
#include "RootSignature.h"

class CameraComponent : public SceneComponent
{
  public:
    CameraComponent(Actor* owner);
    virtual ~CameraComponent();

    void Initialize(const float& fovDegrees, const UINT& width, const UINT& height, const float& nearZ,
                    const float& farZ, bool isPerspective = true);

    void Bind(const RootSignature* rs, ID3D12GraphicsCommandList* cl) const;

  public:
    DirectX::SimpleMath::Matrix GetProjMatrix() const override;
    DirectX::SimpleMath::Matrix GetViewMatrix() const override;

    Matrix CreateProjMatrix() const;

  public:
    ConstantBuffer<GlobalConstant> m_gcb;
    bool m_gcbInitialized = false;

  public:
    void SyncCB();
    void OnRegister() override;
    D3D12_GPU_VIRTUAL_ADDRESS GetGCBGPUAddress() const;

  public:
    void UpdateCameraInfo(const int& width, const int& height);
    void UpdateConstantTransform() override;

  private:
    float m_fovRadians;
    float m_nearZ;
    float m_farZ;
    float m_aspectRatio;
    UINT m_width;
    UINT m_height;
    bool m_perspectiveMode = true;
};
