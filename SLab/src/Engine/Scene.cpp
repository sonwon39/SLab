#include "Scene.h"

#include "imgui.h"

#include "World.h"
#include "RenderEngine.h"
#include "Actors/ACamera.h"
#include "Actors/ALight.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SceneComponent.h"
#include "SPH/SPH.h"
#include "StableFluids/StableFluids.h"
#include "FlipWater/FlipWater.h"
#include "Noise/Noise.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;

// === Scene 공통 ===

void Scene::AddActor(std::shared_ptr<Actor> actor)
{
    m_actors.push_back(std::move(actor));
}

void Scene::RegisterActors()
{
    for (auto& a : m_actors)
    {
        a->OnRegister();
    }
}

void Scene::Render(RenderEngine& re)
{
    re.RenderMeshes();
}

// === PBRScene ===

void PBRScene::Load(World& world)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle;
    if (world.FindTextureHandle("SkyBrdf", handle))
        world.GetIBL()->Initialize(handle);

    ActorData ad = {};
    ad.lc.model = Matrix(DirectX::XMMatrixTranslation(-5.5f, 1.f, 0.f)).Transpose();
    ad.psoName = "pbrPSO";
    ad.textureName = "Metal052C_4K-PNG_albedo";
    ad.useMaterial = true;
    ad.mc.texTransform = Matrix(DirectX::XMMatrixScaling(2.5f, 2.5f, 1.f)).Transpose();
    ad.mc.heightScale = 0.05f;
    ad.mc.useHeightMap = true;
    ad.mc.useNormalMap = true;
    ad.mc.useMetallicMap = true;
    ad.mc.useRoughnessMap = true;
    // gui 조작용
    m_pbrActor = world.GenerateActor("pbr_sphere", ad);

    ad.lc.model = Matrix(DirectX::XMMatrixTranslation(5.5f, 1.f, 0.f)).Transpose();
    ad.textureName = "worn-painted-metal_albedo";
    world.GenerateActor("pbr_sphere", ad);

    ad.mc.useHeightMap = false;
    ad.lc.model = Matrix(DirectX::XMMatrixTranslation(0.f, 0.f, 0.f)).Transpose();
    ad.mc.texTransform = Matrix(DirectX::XMMatrixScaling(15.f, 15.f, 1.f)).Transpose();

    ad.textureName = "PavingStones145_2K-PNG_Albedo";
    world.GenerateActor("pbr_plane", ad);

    ad.lc.model = Matrix(DirectX::XMMatrixTranslation(10.5f, 1.f, 0.f)).Transpose();
    ad.textureName = "Metal052C_4K-PNG_albedo";
    ad.useMaterial = true;
    ad.mc.texTransform = Matrix(DirectX::XMMatrixScaling(1.f, 1.f, 1.f)).Transpose();
    world.GenerateActor("shield", ad);

    ad.lc.model = Matrix(DirectX::XMMatrixTranslation(0.f, 0.f, 0.f)).Transpose();
    ad.textureName = "Michelle_albedo";
    ad.useMaterial = true;
    ad.mc.texTransform = Matrix(DirectX::XMMatrixScaling(1.f, 1.f, 1.f)).Transpose();
    ad.psoName = "skinnedPSO";
    ad.updateConstants = true;
    world.GenerateActor("Capoeira", ad);

    auto camera = std::make_shared<ACamera>();
    camera->Initialize(Vector3(0.f, 2.f, -2.f), true);
    world.AddActor(camera);

    auto light = std::make_shared<ALight>();
    light->Initialize();
    world.AddActor(light);

    ad.textureName = "SkyEnvHDR_CubeMap";
    ad.psoName = "cubeMapPSO";
    world.GenerateActor("simple_cube", ad);

    m_player = camera;

    RegisterActors();
}

void PBRScene::UpdateGUI()
{
    if (!m_pbrActor)
        return;

    MaterialConstant material = m_pbrActor->GetMaterialConstant();

    bool metalic = material.useMetallicMap;
    bool roghness = material.useRoughnessMap;
    bool normal = material.useNormalMap;

    ImGui::SliderFloat("roughness", &material.roughness, 0.f, 1.f);
    ImGui::SliderFloat("metalic", &material.metallic, 0.f, 1.f);

    if (ImGui::Checkbox("Use MetallicMap", &metalic))
        material.useMetallicMap = metalic;
    if (ImGui::Checkbox("Use Roughness", &roghness))
        material.useRoughnessMap = roghness;
    if (ImGui::Checkbox("Use NormalMap", &normal))
        material.useNormalMap = normal;

    m_pbrActor->SetMaterialConstant(material);
}

// === StableFluidsScene (기존 useSimulation 분기 + StableFluidsTick) ===

void StableFluidsScene::Load(World& world)
{
    ActorData ad = {};
    ad.lc.model = DirectX::XMMatrixTranslation(0.f, 0.f, 0.f);
    ad.lc.model = ad.lc.model.Transpose();
    ad.textureName = "sf_density";
    ad.psoName = "defaultPSO";
    ad.useMaterial = false;
    world.GenerateActor("rect", ad);

    auto orthogonalCamera = std::make_shared<ACamera>();
    orthogonalCamera->Initialize(Vector3(0.f, 0.f, 0.f), false);
    world.AddActor(orthogonalCamera);

    m_player = orthogonalCamera;

    RegisterActors();
}

void StableFluidsScene::Simulate(float deltaTime, ID3D12CommandQueue* queue)
{
    m_sim->Tick(deltaTime);
    m_sim->Execute(queue);
}

// === NoiseScene (기존 useNoise 분기 + NoiseSimulationTick) ===

void NoiseScene::Load(World& world)
{
    ActorData ad = {};
    ad.lc.model = DirectX::XMMatrixTranslation(0.f, 0.f, 0.f);
    ad.lc.model = ad.lc.model.Transpose();
    ad.textureName = "noiseDensity";
    ad.psoName = "defaultPSO";
    ad.useMaterial = false;
    world.GenerateActor("rect", ad);

    auto orthogonalCamera = std::make_shared<ACamera>();
    orthogonalCamera->Initialize(Vector3(0.f, 0.f, 0.f), false);
    world.AddActor(orthogonalCamera);

    m_player = orthogonalCamera;

    RegisterActors();
}

void NoiseScene::Simulate(float deltaTime, ID3D12CommandQueue* queue)
{
    m_sim->ResetCommand();
    m_sim->CurlNoiseSimulation(deltaTime);
    m_sim->Execute(queue);
}

// === SPHScene (기존 SPHTick) ===

void SPHScene::Load(World& world)
{
    // SPH 파티클 렌더는 자체 CB를 쓰므로 메시 액터는 없고, 3D 관찰용 카메라만 둔다.
    auto camera = std::make_shared<ACamera>();
    camera->Initialize(Vector3(0.f, 2.f, -2.f), true);
    world.AddActor(camera);

    m_player = camera;

    RegisterActors();
}

void SPHScene::Simulate(float deltaTime, ID3D12CommandQueue* queue)
{
    m_sim->Tick(deltaTime);
    m_sim->Execute(queue);
}

void SPHScene::Render(RenderEngine& re)
{
    re.RenderSPH("particleRenderPSO", true /*clear RT*/);
}

void FlipWaterScene::Load(World& world)
{
    // Particle render uses its own CB (no mesh actors); just a 3D observation camera.
    // Positioned outside the -x/-z corner, aimed into the +x/+z tank walls & down (3/4 view).
    auto camera = std::make_shared<ACamera>();
    camera->Initialize(Vector3(-1.15f, 1.35f, -1.15f), true); // pulled back from the -x/-z corner of the [0,1] tank
    world.AddActor(camera);

    m_player = camera;

    if (SceneComponent* root = camera->GetRootComponent())
        root->UpdateRotation(45.f, 30.f); // yaw into +x/+z corner, pitch down (degrees)

    RegisterActors();
}

void FlipWaterScene::Simulate(float deltaTime, ID3D12CommandQueue* queue)
{
    m_sim->Tick(deltaTime);
    m_sim->Execute(queue);
}

void FlipWaterScene::Render(RenderEngine& re)
{
    re.RenderFluid(m_viewMode);
}

void FlipWaterScene::UpdateGUI()
{
    // Drawn inside RenderEngine's "GUI" window (see UpdateGUI()).
    if (ImGui::Button("Reset"))
        m_sim->Reset();

    static const char* kModes[] = {"1 particles", "2 raw depth", "3 smooth depth",
                                   "4 thickness", "5 normals", "6 water"};
    int idx = m_viewMode - 1;               // 1..6 -> 0..5
    if (ImGui::Combo("View", &idx, kModes, IM_ARRAYSIZE(kModes)))
        m_viewMode = idx + 1;
}
