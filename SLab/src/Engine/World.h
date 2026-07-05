#pragma once

#include "d3d12.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "directxtk12\SimpleMath.h"

#include "InputHelper/InputHelper.h"
#include "InputHelper/SEngineMouse.h"

#include "Core/Texture2D.h"
#include "DescriptorHeap.h"
#include "StableFluids/Grid.h"
#include "Core/ConstantBuffer.h"
#include "Core/GPUBuffer.h"
#include "Level.h"
#include "Scene.h"
#include "Material.h"
#include "AssetManager/TextureLoader.h"
#include "AssetManager/ModelLoader.h"
#include "AssetManager/LightManager.h"
#include "GameFramework/Actor.h"
#include "IBLEnvironment.h"
#include "ImageInfo.h"

class World
{
  public:
    World();
    virtual ~World();

  public:
    // model loader 및 마우스 초기화
    void Initialize(ID3D12Device5* device, int width, int height);
    void Tick(float deltaTime);

    bool FindTexture(const std::string& textureName, int& index);
    bool FindTextureHandle(const std::string& textureName, D3D12_GPU_DESCRIPTOR_HANDLE& handle);
    void AddTexture(const std::string& textureName, Texture2D& texture);
    
  public:
    // 신 등록 + 기본 신 로드 RenderEngine::InitScene 이후 호출.
    void InitScenes(RenderEngine& re);
    void RegisterScene(std::unique_ptr<Scene> scene);
    // 신 전환 요청. 실제 전환은 프레임 경계(RenderEngine::Tick 첫머리)에서 일어난다.
    void RequestScene(const std::string& name);
    // pending 신을 활성화. 매 프레임 Execute가 GPU를 flush하므로 프레임 경계 호출은 안전.
    void ApplyPendingScene(RenderEngine& re);
    Scene* GetActiveScene() const
    {
        return m_activeScene;
    }
    const std::vector<std::string>& GetSceneNames() const
    {
        return m_sceneNames;
    }
    // 상주하는 모든 신의 카메라 화면비 갱신 (리사이즈 시)
    void UpdateCamerasOnResize(int width, int height);

  public:
    ID3D12Device5* GetDevice()
    {
        return m_device;
    }
    std::shared_ptr<TextureLoader> GetTextureLoader() const
    {
        return m_textureLoader;
    }
    std::shared_ptr<ModelLoader<Vertex, uint16_t>> GetModelLoader() const
    {
        return m_modelLoader;
    }
    std::shared_ptr<PBRModelLoader> GetPBRModelLoader() const
    {
        return m_pbrModelLoader;
    }
    std::shared_ptr<SkinnedModelLoader> GetSkinnedModelLoader() const
    {
        return m_skinnedModelLoader;
    }
    std::shared_ptr<SimpleModelLoader> GetSimpleModelLoader() const
    {
        return m_simpleModelLoader;
    }
    std::shared_ptr<LightManager> GetLightManger() const
    {
        return m_lightManager;
    }
    std::shared_ptr<GPUBuffer> GetReadBackBuffer() const
    {
        return m_readbackBuffer;
    }
    ID3D12DescriptorHeap* GetMainHeap() const;
    Vector2 GetMouseRawDelta() const;

  public:
    void SetWindowSize(int width, int height);

  public:
    void SetFPSMode(bool newState)
    {
        m_fpsMode = newState;
        if (m_fpsMode)
        {
            MoveMouseToWindowCenter();
        }
    }
    bool GetFPSMode() const
    {
        return m_fpsMode;
    }

  public:
    std::shared_ptr<StaticMesh> GetMesh(const std::string& meshName);
    std::shared_ptr<Actor> GenerateActor(const std::string& meshName, const ActorData& ad);
    // 현재 활성(로딩 중 포함) 신에 액터 추가
    void AddActor(std::shared_ptr<Actor> actor);
    void AddMesh(const std::string& meshName, std::shared_ptr<StaticMesh> mesh);

    // mesh 내의 vertex index buffer blob 제거
    void ClearMeshBlobs();
    void ClearTextureBlobs();

    void MoveMouseToWindowCenter();

    POINT GetCenterPoint();

    void InitReadbackBuffer(UINT64 size);

    // 텍스처 이름으로 Material을 얻는다. 같은 이름이면 같은 Material을 반환(캐시).
    // 텍스처가 로드돼 있지 않으면 nullptr.
    std::shared_ptr<Material> GetOrCreateMaterial(const std::string& textureName);
    std::shared_ptr<Actor> GetPlayer() const
    {
        return m_activeScene ? m_activeScene->GetPlayer() : nullptr;
    }
    std::shared_ptr<IBLEnvironment> GetIBL() const
    {
        return m_iblEnv;
    }

  private:
    void SaveLoop();

  public:
    // readback 버퍼를 매핑해 f16→8bit RGBA(톤매핑 포함)로 변환. PNG 저장/영상 인코딩 공용.
    void ReadbackToRGBA(const ImageInfo& info, std::vector<uint8_t>& out);
    // outPath: 저장할 파일 전체 경로(폴더 포함). 동기 호출 가능(녹화 시 렌더 스레드에서 직접 사용).
    void SaveTextureCPU(const ImageInfo& info, const std::string& outPath);
    void Notify(const ImageInfo& info);

  public:
    UINT m_cbvSrvDescriptorSize = 0;
    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;

  public:
    HWND m_mainWnd;
    std::shared_ptr<SEngineMouse> mouse;
    std::shared_ptr<InputHelper> m_inputHelper;
    UINT windowWidth;
    UINT windowHeight;
    std::vector<DirectX::SimpleMath::Vector3> colors;

  private:
    ID3D12Device5* m_device;

  public:
    bool m_fpsMode = false;

  private:
    std::shared_ptr<Level> m_level;

  private:
    std::string texBuildPath = "Assets/Build/";
    std::shared_ptr<TextureLoader> m_textureLoader;
    std::string skyTextureName = "Sky";

  private:
    std::shared_ptr<ModelLoader<Vertex, uint16_t>> m_modelLoader;
    std::shared_ptr<SimpleModelLoader> m_simpleModelLoader;
    std::shared_ptr<PBRModelLoader> m_pbrModelLoader;
    std::shared_ptr<SkinnedModelLoader> m_skinnedModelLoader;

  private:
    std::shared_ptr<LightManager> m_lightManager;

  private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    std::vector<std::string> m_sceneNames; // GUI 표시 순서 (등록 순)
    Scene* m_activeScene = nullptr;
    std::string m_pendingSceneName;

    std::shared_ptr<IBLEnvironment> m_iblEnv;

    std::unordered_map<std::string, std::shared_ptr<StaticMesh>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;

    // save 용
  private:
    std::shared_ptr<GPUBuffer> m_readbackBuffer;
    std::thread saveThread;
    bool saveFlag = false;
    bool stopThread = false;
    std::mutex saveMtx;
    std::condition_variable saveCv;

	ImageInfo sharedInfo;
};
