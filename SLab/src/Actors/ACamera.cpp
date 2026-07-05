#include "ACamera.h"
#include "GraphicsCommon.h"
#include "Engine\World.h"
#include "GameFramework\StaticMeshComponent.h"
#include "GameFramework\CameraComponent.h"

using namespace Graphics;

ACamera::ACamera()
{
    m_velocity = 10.f;
}

ACamera::~ACamera()
{
}

void ACamera::Initialize(DirectX::SimpleMath::Vector3& location, bool isPerspective)
{
    if (!m_world)
        return;
    auto mesh = m_world->GetMesh("sphere");

    LocalConstant lc;
    lc.model = DirectX::XMMatrixTranslation(location.x, location.y, location.z);
    lc.model = lc.model.Transpose();

    std::shared_ptr<StaticMeshComponent> root = std::make_shared<StaticMeshComponent>(this);
    root->SetMesh(mesh);
    root->SetUpdateConstant(true);
    root->SetLocalConstant(lc);
    root->SetTextureName("PavingStones145_2K-PNG_Albedo");
    root->SetPSOName("defaultPSO");

    std::shared_ptr<CameraComponent> camera = std::make_shared<CameraComponent>(this);
    root->Attach(camera);
    camera->Initialize(70.f, m_world->windowWidth, m_world->windowHeight, 0.01f, 500.f, isPerspective);

    SetRootComponent(root);
}

void ACamera::Tick(const float& deltaTime)
{
    if (!m_world || !(m_world->m_inputHelper))
        return;

    Vector3 dir = m_world->m_inputHelper->GetInputDirection(this);
    Vector3 dx = dir * m_velocity * deltaTime;

    Vector2 cameraDel = m_world->GetMouseRawDelta();

    Actor::UpdateActorLocation(dx);

    if (m_world->GetFPSMode())
        Actor::UpdateRotation((int)cameraDel.x, (int)cameraDel.y, deltaTime);
}
