#pragma once

#include <memory>
#include <string>
#include <vector>

#include "d3d12.h"

class Actor;
class World;
class RenderEngine;
class MeshBatch;
class CameraComponent;
class SPH;
class StableFluids;
class Noise;
class FlipWater;

// 한 신의 렌더 등록 상태.
// 컴포넌트 OnRegister → RenderEngine::RegistMeshBatch/RegistCamera 가
// "현재 로딩 중인 신"의 RenderScene 으로 밀어넣는다 (기존 전역 vector 대체).
// MeshBatch/카메라 수명 = 이 신의 액터 수명이므로 raw owner 포인터가 안전해진다.
struct RenderScene
{
    std::vector<std::shared_ptr<MeshBatch>> meshBatchs;
    std::vector<CameraComponent*> cameras;
};

// 신 = 액터 구성 + (선택) 시뮬레이션 + 렌더 경로.
// 모든 신은 초기화 때 생성되어 상주하고, Load 는 최초 활성화 시 1회만 호출된다.
// 신 전환은 프레임 경계에서만 일어난다 (World::ApplyPendingScene 참고).
class Scene
{
  public:
    explicit Scene(const std::string& name) : m_name(name)
    {
    }
    virtual ~Scene() = default;

  public:
    // 액터/카메라 생성 + OnRegister.
    // 호출 전 RenderEngine 의 등록 타깃이 이 신의 RenderScene 으로 세팅돼 있어야 한다.
    virtual void Load(World& world) = 0;
    // 시뮬레이션 커맨드 기록 + 큐 제출 (기본: 시뮬 없음)
    virtual void Simulate(float deltaTime, ID3D12CommandQueue* queue)
    {
    }
    // 백버퍼에 신 내용 기록 (기본: 공용 메시 렌더 경로)
    virtual void Render(RenderEngine& re);
    // 신별 ImGui 컨트롤 (기본: 없음)
    virtual void UpdateGUI()
    {
    }

  public:
    const std::string& GetName() const
    {
        return m_name;
    }
    RenderScene& GetRenderScene()
    {
        return m_renderScene;
    }
    bool IsLoaded() const
    {
        return m_loaded;
    }
    void SetLoaded()
    {
        m_loaded = true;
    }

    void AddActor(std::shared_ptr<Actor> actor);
    std::shared_ptr<Actor> GetPlayer() const
    {
        return m_player;
    }
    std::vector<std::shared_ptr<Actor>>& GetActors()
    {
        return m_actors;
    }

  protected:
    // m_actors 전체 OnRegister — Load 마지막에 호출
    void RegisterActors();

  protected:
    std::string m_name;
    RenderScene m_renderScene;
    std::vector<std::shared_ptr<Actor>> m_actors;
    std::shared_ptr<Actor> m_player;
    bool m_loaded = false;
};

// PBR + IBL + 스켈레탈 애니메이션 (기존 renderDefault 분기)
class PBRScene : public Scene
{
  public:
    PBRScene() : Scene("PBR")
    {
    }
    void Load(World& world) override;
    void UpdateGUI() override;

  private:
    // GUI material 조작 대상 (기존 World::m_pbr)
    std::shared_ptr<Actor> m_pbrActor;
};

// 2D 연기 (기존 useSimulation 분기)
class StableFluidsScene : public Scene
{
  public:
    explicit StableFluidsScene(std::shared_ptr<StableFluids> sim) : Scene("StableFluids"), m_sim(sim)
    {
    }
    void Load(World& world) override;
    void Simulate(float deltaTime, ID3D12CommandQueue* queue) override;

  private:
    std::shared_ptr<StableFluids> m_sim;
};

// Curl-noise 파티클 (기존 useNoise 분기)
class NoiseScene : public Scene
{
  public:
    explicit NoiseScene(std::shared_ptr<Noise> sim) : Scene("CurlNoise"), m_sim(sim)
    {
    }
    void Load(World& world) override;
    void Simulate(float deltaTime, ID3D12CommandQueue* queue) override;

  private:
    std::shared_ptr<Noise> m_sim;
};

class SPHScene : public Scene
{
  public:
    explicit SPHScene(std::shared_ptr<SPH> sim) : Scene("SPH"), m_sim(sim)
    {
    }
    void Load(World& world) override;
    void Simulate(float deltaTime, ID3D12CommandQueue* queue) override;
    void Render(RenderEngine& re) override;

  private:
    std::shared_ptr<SPH> m_sim;
};

// Fluids
class FlipWaterScene : public Scene
{
  public:
    explicit FlipWaterScene(std::shared_ptr<FlipWater> sim) : Scene("FlipWater"), m_sim(sim)
    {
    }
    void Load(World& world) override;
    void Simulate(float deltaTime, ID3D12CommandQueue* queue) override;
    void Render(RenderEngine& re) override;
    void UpdateGUI() override;

  private:
    std::shared_ptr<FlipWater> m_sim;
    int m_viewMode = 6; // 1 particles, 2 rawZ, 3 smoothZ, 4 thick, 5 normals, 6 water
};
