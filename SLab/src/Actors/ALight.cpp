#include "ALight.h"
#include "GraphicsCommon.h"
#include "Engine\World.h"
#include "GameFramework\StaticMeshComponent.h"
#include "GameFramework\LightComponent.h"

using namespace Graphics;

ALight::ALight()
{
}

ALight::~ALight()
{
}
void ALight::Initialize()
{
    if (!m_world)
        return;
    auto mesh = m_world->GetMesh("sphere");
    Light l;
    l.position = Vector3(70.f, -70.f, 0.f);
    origin = l.position;
    target = Vector3(10.f, -10.f, 0.f);
    len = (target - origin).Length();
    dir = -origin;
    dir.Normalize();
    originDir = dir;

    m_velocity = 10.f;

	l.fallOffEnd = 100.f;
    l.radiance = Vector3(1.f, 1.f, 1.f);
    l.enabled = 1;

    LocalConstant lc;
    lc.model = DirectX::XMMatrixTranslation(l.position.x, l.position.y, l.position.z);
    lc.model = lc.model.Transpose();

    std::shared_ptr<StaticMeshComponent> root = std::make_shared<StaticMeshComponent>(this);
    root->SetMesh(mesh);
    root->SetUpdateConstant(true);
    root->SetLocalConstant(lc);
    root->SetTextureName("PavingStones145_2K-PNG_Albedo");
    root->SetPSOName("defaultPSO");

    std::shared_ptr<LightComponent> light = std::make_shared<LightComponent>(this);
    light->Initialize(l);

    root->Attach(light);
     SetRootComponent(root);
}

void ALight::Tick(const float& deltaTime)
{
    Vector3 currPos = GetActorLocation();
    Vector3 del = dir * deltaTime * m_velocity;
    Vector3 nextPos = currPos + del;
	// 0 보다 더 진행한 경우
	if ((nextPos - origin).Length() > len)
	{
        SetActorLocation(target);
        dir = -dir;
	}
	else if ((nextPos).Length() > origin.Length())
	{
        SetActorLocation(origin);
        dir = -dir;
	}
    else
		UpdateActorLocation(del);
}
