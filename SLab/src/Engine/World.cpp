#pragma warning(disable : 4996)

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "World.h"
#include "RenderEngine.h"
#include "Actors/AMovingPlatform.h"
#include "Actors/ACamera.h"
#include "Actors/ALight.h"
#include "GameFramework/CameraComponent.h"
#include <fp16.h>

World::World()
{
    mouse = std::make_shared<SEngineMouse>();
    m_inputHelper = std::make_shared<InputHelper>();

    m_level = std::make_shared<Level>();
    m_textureLoader = std::make_shared<TextureLoader>(texBuildPath);
    m_modelLoader = std::make_shared<ModelLoader<Vertex, uint16_t>>();
    m_simpleModelLoader = std::make_shared<SimpleModelLoader>();
    m_pbrModelLoader = std::make_shared<PBRModelLoader>();
    m_skinnedModelLoader = std::make_shared<SkinnedModelLoader>();

    m_lightManager = std::make_shared<LightManager>();

    m_iblEnv = std::make_shared<IBLEnvironment>();

    m_readbackBuffer = std::make_shared<GPUBuffer>();

	saveThread = std::thread(&World::SaveLoop, this);
}

World::~World()
{
    mouse.reset();
    m_level.reset();
    m_textureLoader.reset();
	{
        std::lock_guard<std::mutex> lock(saveMtx);
        stopThread = true;
    }
    saveCv.notify_one();
    if (saveThread.joinable())
        saveThread.join();
}

void World::Initialize(ID3D12Device5* device, int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    m_device = device;

    m_modelLoader->InitializeCPU();
    m_simpleModelLoader->InitializeCPU();
    m_pbrModelLoader->InitializeCPU();
    m_skinnedModelLoader->InitializeCPU();

    auto scalingTr = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);
	auto unrealFrontTR = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f) * DirectX::XMMatrixRotationY(DirectX::XM_PI);
    m_pbrModelLoader->Load("shield.fbx", scalingTr, false);
    //m_pbrModelLoader->Load("Animations/Capoeira.fbx", scalingTr, true);
    m_skinnedModelLoader->Load("Animations/Capoeira.fbx", scalingTr, true);

    m_inputHelper->Initialize();

    SetWindowSize(width, height);

    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    mouse->Initilize();

    colors = {
        Vector3(0.3f, 0.8f, 0.4f), // Light Green
        Vector3(1.0f, 0.0f, 0.0f), // Red
        Vector3(0.0f, 1.0f, 0.0f), // Green
        Vector3(0.0f, 0.0f, 1.0f), // Blue
        Vector3(1.0f, 1.0f, 0.0f), // Yellow
        Vector3(1.0f, 0.0f, 1.0f), // Magenta
        Vector3(0.0f, 1.0f, 1.0f), // Cyan
        Vector3(1.0f, 0.5f, 0.0f), // Orange
        Vector3(0.5f, 0.0f, 1.0f), // Purple
        Vector3(0.8f, 0.8f, 0.8f)  // Light Gray
    };

    // InitReadbackBuffer();
}

void World::Tick(float deltaTime)
{
    if (mouse)
    {
        mouse->Tick(deltaTime);
    }

    if (m_activeScene)
    {
        for (auto& actor : m_activeScene->GetActors())
        {
            actor->Tick(deltaTime);
            actor->TickComponents(deltaTime);
        }
    }

    m_lightManager->Update();
}

bool World::FindTexture(const std::string& textureName, int& index)
{
    if (!m_textureLoader)
        return false;

    index = m_textureLoader->GetTextureIndex(textureName);
    if (index == -1)
        return false;

    return true;
}

bool World::FindTextureHandle(const std::string& textureName, D3D12_GPU_DESCRIPTOR_HANDLE& handle)
{
    int index;
    if (FindTexture(textureName, index))
    {
        handle = m_textureLoader->GetGPUHandle(index);
        return true;
    }
    return false;
}

void World::AddTexture(const std::string& textureName, Texture2D& texture)
{
    if (!m_textureLoader)
        return;

    m_textureLoader->AddTexture(textureName, texture);
}

void World::InitScenes(RenderEngine& re)
{
	RegisterScene(std::make_unique<PBRScene>());
    //RegisterScene(std::make_unique<SPHScene>(re.GetSPH()));
    RegisterScene(std::make_unique<StableFluidsScene>(re.GetStableFluids()));
    RegisterScene(std::make_unique<NoiseScene>(re.GetNoise()));
    RegisterScene(std::make_unique<FlipWaterScene>(re.GetFlipWater()));

	RequestScene("FlipWater"); // Step 1 (숫자키 1~4 로 신 전환)
    ApplyPendingScene(re); // 첫 프레임 전에 즉시 로드
}

void World::RegisterScene(std::unique_ptr<Scene> scene)
{
    m_sceneNames.push_back(scene->GetName());
    m_scenes[scene->GetName()] = std::move(scene);
}

void World::RequestScene(const std::string& name)
{
    m_pendingSceneName = name;
}

void World::ApplyPendingScene(RenderEngine& re)
{
    if (m_pendingSceneName.empty())
        return;

    auto it = m_scenes.find(m_pendingSceneName);
    m_pendingSceneName.clear();
    if (it == m_scenes.end())
        return;

    Scene* next = it->second.get();
    if (next == m_activeScene)
        return;

    // Load 중 GenerateActor → AddActor 가 이 신으로 들어가도록 먼저 교체
    m_activeScene = next;

    if (!next->IsLoaded())
    {
        // 컴포넌트 OnRegister(RegistMeshBatch/RegistCamera)가 이 신의 RenderScene에 쌓이게 한다
        re.SetRegistrationTarget(&next->GetRenderScene());
        next->Load(*this);
        re.SetRegistrationTarget(nullptr);
        next->SetLoaded();
    }

    std::cout << "[Scene] active=" << next->GetName() << " meshBatchs=" << next->GetRenderScene().meshBatchs.size()
              << " cameras=" << next->GetRenderScene().cameras.size() << std::endl;
}

void World::UpdateCamerasOnResize(int width, int height)
{
    for (auto& [name, scene] : m_scenes)
    {
        for (auto& camera : scene->GetRenderScene().cameras)
        {
            camera->UpdateCameraInfo(width, height);
        }
    }
}

ID3D12DescriptorHeap* World::GetMainHeap() const
{
    if (!m_textureLoader)
        return nullptr;

    return m_textureLoader->GetDescriptorHeap()->GetHeap();
}

Vector2 World::GetMouseRawDelta() const
{
    Vector2 delta;
    if (mouse)
    {
        delta = mouse->GetMouseRawDelta();
        mouse->SetRawDelta(Vector2(0.f, 0.f));
    }
    return delta;
}

std::shared_ptr<Material> World::GetOrCreateMaterial(const std::string& textureName)
{
    auto it = m_materials.find(textureName);
    if (it != m_materials.end())
        return it->second;

    int index;
    if (!FindTexture(textureName, index))
        return nullptr;

    auto mat = std::make_shared<Material>();
    mat->Initialize(m_textureLoader->GetGPUHandle(index));
    m_materials[textureName] = mat;
    return mat;
}

void World::SetWindowSize(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

}

std::shared_ptr<StaticMesh> World::GetMesh(const std::string& meshName)
{
    auto it = m_meshes.find(meshName);
    if (it == m_meshes.end())
        return nullptr;

    return it->second;
}

std::shared_ptr<Actor> World::GenerateActor(const std::string& meshName, const ActorData& ad)
{
    auto mesh = GetMesh(meshName);
    if (!mesh)
        return nullptr;

    std::shared_ptr<Actor> actor = std::make_shared<Actor>();
    actor->Initialize(mesh, ad);
    AddActor(actor);
    return actor;
}

void World::AddActor(std::shared_ptr<Actor> actor)
{
    if (m_activeScene)
        m_activeScene->AddActor(std::move(actor));
}

void World::AddMesh(const std::string& meshName, std::shared_ptr<StaticMesh> mesh)
{
    m_meshes[meshName] = mesh;
}

void World::ClearMeshBlobs()
{
    for (auto& m : m_meshes)
    {
        m.second->Clear();
    }
}

void World::ClearTextureBlobs()
{
    m_textureLoader->ClearBlobs();
}

void World::MoveMouseToWindowCenter()
{
    POINT center = GetCenterPoint();

    // 클라이언트 좌표 → 화면 좌표
    ClientToScreen(m_mainWnd, &center);

    // 마우스 이동
    SetCursorPos(center.x, center.y);
}

POINT World::GetCenterPoint()
{
    RECT rect;
    GetClientRect(m_mainWnd, &rect);

    // 클라이언트 영역 중앙 좌표
    POINT center;
    center.x = (rect.left + rect.right) / 2;
    center.y = (rect.top + rect.bottom) / 2;
    return center;
}

void World::InitReadbackBuffer(UINT64 size)
{
    m_readbackBuffer->Initialize(size, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST,
                                 D3D12_RESOURCE_FLAG_NONE, L"readback");
    m_readbackBuffer->SetResourceStates(D3D12_RESOURCE_STATE_COPY_DEST);
}

// TODO : 현재 f16 텍스쳐만 가능
void World::ReadbackToRGBA(const ImageInfo& info, std::vector<uint8_t>& out)
{
    if (!m_readbackBuffer)
        return;

    uint32_t pixelCount = (uint32_t)(info.rowSize * info.numRows / 2);
    std::vector<uint16_t> imagef16(pixelCount);
    out.resize(pixelCount);

    m_readbackBuffer->MapForRead();
    m_readbackBuffer->CopyToCpu(imagef16.data(), info.numRows, info.rowSize, info.rowPitch);

    for (size_t i = 0; i < out.size(); i++)
    {
        double c = std::clamp(fp16_ieee_to_fp32_value(imagef16[i]), 0.f, 1.f);
        // tone mapping (alpha 채널 제외)
        if ((i + 1) % 4 != 0)
            c = std::pow(c, 1 / 2.2);
        out[i] = std::clamp((int)(c * 255.f), 0, 255);
    }
}

void World::SaveTextureCPU(const ImageInfo& info, const std::string& outPath)
{
    std::vector<uint8_t> image;
    ReadbackToRGBA(info, image);
    if (image.empty())
        return;

    stbi_write_png(outPath.c_str(), (int)info.width, (int)info.height, 4, image.data(), (int)(info.rowSize / 2));
}

void World::SaveLoop()
{
	while (true)	{
        ImageInfo local;
        {
            std::unique_lock lock(saveMtx);
            saveCv.wait(lock, [this] { return stopThread || saveFlag; });
            if (stopThread)
                break;

			local = sharedInfo;
            saveFlag = false;
        }
        std::string fileFullPath = local.resultPath;
        SaveTextureCPU(local, fileFullPath);
	}
}
void World::Notify(const ImageInfo& info)
{
	{
        std::lock_guard<std::mutex> lock(saveMtx);
        sharedInfo = info;
        saveFlag = true;
	}
    saveCv.notify_one();
}
