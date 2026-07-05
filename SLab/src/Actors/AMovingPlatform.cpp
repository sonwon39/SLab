#include "AMovingPlatform.h"
#include "GraphicsCommon.h"
#include "Engine\World.h"
#include "GameFramework\StaticMeshComponent.h"
#include "GameFramework\CameraComponent.h"
#include "ActorComponents\Mover.h"


using namespace Graphics;

AMovingPlatform::AMovingPlatform()
{
    m_velocity = 10.f;
}

AMovingPlatform::~AMovingPlatform()
{
}

void AMovingPlatform::Initialize()
{
    if (!m_world)
        return;
    auto mesh = m_world->GetMesh("cube");

    LocalConstant lc;
    lc.model = DirectX::XMMatrixTranslation(2.f, 1.f, 3.f);
    lc.model = lc.model.Transpose();

    std::shared_ptr<StaticMeshComponent> root = std::make_shared<StaticMeshComponent>(this);
    root->SetMesh(mesh);
    root->SetUpdateConstant(true);
    root->SetLocalConstant(lc);
    root->SetTextureName("PavingStones145_2K-PNG_Albedo");
    root->SetPSOName("defaultPSO");

    SetRootComponent(root);

	std::shared_ptr<Mover> mover = std::make_shared<Mover>(this);
    mover->Initialize();

	AddActorComponent(mover);
	
}

void AMovingPlatform::Tick(const float& deltaTime)
{
  /*  if (!m_world || !(m_world->m_inputHelper))
        return;*/

    /*Vector3 dir = m_world->m_inputHelper->GetInputDirection();
    Vector3 dx = dir * m_velocity *deltaTime;
    Actor::UpdateActorLocation(dx);*/
}
