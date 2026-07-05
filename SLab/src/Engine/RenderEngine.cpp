#pragma warning(disable : 4996)

#include "Directxtk12/DDSTextureLoader.h"
#include "directxtk12/ResourceUploadBatch.h"

#include <pix3.h>
#include <random>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include "RenderEngine.h"
#include "GraphicsCommon.h"
#include "World.h"

#include "RootSignature.h"
#include "PipelineState.h"
#include "GeometryGenerator.h"

#include "StaticMesh.h"
#include "AssetManager/TextureLoader.h"
#include "AssetManager/ModelLoader.h"

#include "GameFramework/CameraComponent.h"
#include "GameFramework/SceneComponent.h"

using Microsoft::WRL::ComPtr;
using namespace GraphicsUtils;
using namespace Graphics;
using namespace Renderer;

static const float PI = 3.141592f;

RenderEngine::RenderEngine(ID3D12Device5* device) : m_device(device)
{
    videoName = "StableFluids";
    m_recording = false;
}

RenderEngine::~RenderEngine()
{
    CloseVideoPipe(); // 녹화 중 종료돼도 mp4를 정상 마무리
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool RenderEngine::Initialize(int width, int height, int guiWidth, IDXGIFactory7* factory)
{
    m_guiWidth = guiWidth;
    m_width = width;
    m_height = height;

    m_viewport = CD3DX12_VIEWPORT((FLOAT)m_guiWidth, 0.F, (FLOAT)(m_width - m_guiWidth), (FLOAT)m_height);
    m_scissorRect = CD3DX12_RECT((LONG)0, 0, (LONG)(m_width), (LONG)m_height);
    m_hdrViewport = CD3DX12_VIEWPORT((FLOAT)0.F, 0.F, (FLOAT)(m_width), (FLOAT)m_height);

    // rtvClearColor = { 0.53F, 0.81F, 0.92F, 1.0F };
    rtvClearColor = {0.F, 0.F, 0.F, 1.0F};
    blackClearColor = {0.F, 0.F, 0.F, 1.0F};

    CreateCommandObjects();

    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_createBufferfence.GetAddressOf()));

    // Descriptor Handle offset 구하기
    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    CreateDescriptorHeaps();
    CreateSwapChain(factory);

    // Create SwapChain RTVs
    for (int i = 0; i < m_swapChainBufferCount; i++)
    {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_swapChainResources[i].ReleaseAndGetAddressOf()));
        m_swapChainRTVHeap.CreateResourceView(m_swapChainResources[i].Get(), DescriptorType::RTV);
        m_swapChainResources[i].SetResourceStates(D3D12_RESOURCE_STATE_PRESENT);
    }

    CreateDepthBuffers();
    CreateFluidTargets();

    InitScene();

    return true;
}

bool RenderEngine::InitScene()
{
    m_resourceCommandAllocator->Reset();
    ThrowIfFailed(m_resourceCommandList->Reset(m_resourceCommandAllocator.Get(), nullptr));

    InitMeshBuffer();
    InitShaderResources();

    // sph 초기화
    {
        m_sph = std::make_shared<SPH>();
        m_sph->Initialize(m_device, m_resourceCommandAllocator.Get(), m_resourceCommandList.Get());
    }
    // stable fluids 초기화
    {
        m_stableFluids = std::make_shared<StableFluids>();
        m_stableFluids->Initialize(m_width, m_height);
    }
	// noise 초기화
    {
        m_noise = std::make_shared<Noise>();
        m_noise->Initialize(m_width, m_height, m_resourceCommandList.Get());
        GenerateNoise();
    }
	{
        m_flipWater = std::make_shared<FlipWater>();
        // seed upload rides the shared resource cmd list (same as SPH), flushed below.
        m_flipWater->Initialize(m_device, m_resourceCommandAllocator.Get(), m_resourceCommandList.Get());
	}
    // light 초기화
    {
        auto light = m_world->GetLightManger();
        light->Initialize(D3D12_RESOURCE_FLAG_NONE, m_resourceCommandList.Get(), L"light");
    }
    m_resourceCommandList->Close();
    ID3D12CommandList* commands[] = {m_resourceCommandList.Get()};
    m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);
    FlushCommands();

    // 텍스쳐, 메쉬 GPU RESOURCE 생성 후 blob 제거
    m_world->ClearTextureBlobs();
    m_world->ClearMeshBlobs();

    cubemapMaterial = m_world->GetOrCreateMaterial("SkyBrdf");

    // camera settings
    localConstant.model = DirectX::SimpleMath::Matrix();
    localConstant.view = DirectX::XMMatrixLookToLH(Vector3(0, 0, -3), Vector3(0, 0, 1), Vector3(0, 1, 0));

    float degrees = 70.f;
    float radians = DirectX::XMConvertToRadians(degrees);
    float aspectRatio = (float)m_width / m_height;
    localConstant.projection = DirectX::XMMatrixPerspectiveFovLH(radians, aspectRatio, 0.1f, 100.f);

    localConstant.model = localConstant.model.Transpose();
    localConstant.view = localConstant.view.Transpose();
    localConstant.projection = localConstant.projection.Transpose();

    utility->CreateConstantBuffer(sizeof(localConstant), m_localCB, &pLocalCB);
    memcpy(pLocalCB, &localConstant, sizeof(localConstant));

    return true;
}

void RenderEngine::InitMeshBuffer()
{
    auto simpleModelLoader = m_world->GetSimpleModelLoader();
    simpleModelLoader->InitializeGPU(m_resourceCommandList.Get());
    auto modelLoader = m_world->GetModelLoader();
    modelLoader->InitializeGPU(m_resourceCommandList.Get());
    auto pbrModelLoader = m_world->GetPBRModelLoader();
    pbrModelLoader->InitializeGPU(m_resourceCommandList.Get());
    auto skinnedModelLoader = m_world->GetSkinnedModelLoader();
    skinnedModelLoader->InitializeGPU(m_resourceCommandList.Get());
}

void RenderEngine::InitShaderResources()
{
    std::shared_ptr<TextureLoader> texLoader;
    // texture 준비
    {
        if (m_world)
        {
            texLoader = m_world->GetTextureLoader();
            texLoader->LoadIdx();
            texLoader->InitHeap(100);
            texLoader->LoadTextures(m_resourceCommandList.Get());
        }
    }
}
bool RenderEngine::InitGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui::StyleColorsLight();
    const char* fontPath = "Assets/Fonts/Hack-Regular.ttf";
    float fontSize = 15.0f;
    // 폰트 로드
    io.Fonts->AddFontFromFileTTF(fontPath, fontSize);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_guiFontHeap));

    // Setup Platform/Renderer backends
    if (m_world)
        ImGui_ImplWin32_Init(m_world->m_mainWnd);

    ImGui_ImplDX12_Init(m_device, m_swapChainBufferCount, Renderer::backBufferFormat, m_guiFontHeap.Get(),
                        m_guiFontHeap->GetCPUDescriptorHandleForHeapStart(),
                        m_guiFontHeap->GetGPUDescriptorHandleForHeapStart());

    return true;
}

// BOOKMARK
void RenderEngine::OnResize(int width, int height)
{
    if (m_swapChain == nullptr)
        return;

    m_width = width;
    m_height = height;

    // swapchain 버퍼 리셋
    for (int i = 0; i < m_swapChainBufferCount; i++)
    {
        m_swapChainResources[i].Reset();
    }

    // swapchain 버퍼 크기 조정
    m_swapChain->ResizeBuffers(m_swapChainBufferCount, m_width, m_height, DXGI_FORMAT_UNKNOWN,
                               DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    // 버퍼에 대한 RTV 재생성
    m_swapChainRTVHeap.ResetIndex();
    for (int i = 0; i < m_swapChainBufferCount; i++)
    {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_swapChainResources[i].ReleaseAndGetAddressOf()));
        m_swapChainRTVHeap.CreateResourceView(m_swapChainResources[i].Get(), DescriptorType::RTV);
        m_swapChainResources[i].SetResourceStates(D3D12_RESOURCE_STATE_PRESENT);
    }

    // DepthBuffer 재생성
    m_dsvHeap.ResetIndex();
    CreateMainDepthBuffer();
    CreateFluidTargets();

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    m_viewport = CD3DX12_VIEWPORT((FLOAT)m_guiWidth, 0.F, (FLOAT)(m_width - m_guiWidth), (FLOAT)m_height);
    m_hdrViewport = CD3DX12_VIEWPORT((FLOAT)0.F, 0.F, (FLOAT)(m_width), (FLOAT)m_height);
    m_scissorRect = CD3DX12_RECT((LONG)0, 0, (LONG)(m_width), (LONG)m_height);

    // 상주하는 모든 신의 카메라 화면비 갱신
    if (m_world)
        m_world->UpdateCamerasOnResize(m_width, m_height);
}

void RenderEngine::CreateCommandObjects()
{
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr,
                                              IID_PPV_ARGS(&m_commandList)));

    ThrowIfFailed(
        m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_resourceCommandAllocator)));

    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_resourceCommandAllocator.Get(),
                                              nullptr, IID_PPV_ARGS(&m_resourceCommandList)));

    D3D12_COMMAND_QUEUE_DESC queueDesc;
    ZeroMemory(&queueDesc, sizeof(queueDesc));
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    m_commandList->Close();
    m_resourceCommandList->Close();
}

void RenderEngine::CreateSwapChain(IDXGIFactory7* factory)
{
    ComPtr<IDXGISwapChain1> swapChain;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_swapChainBufferCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = Renderer::backBufferFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    if (m_world)
    {
        ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_world->m_mainWnd, &swapChainDesc, nullptr,
                                                      nullptr, &swapChain));
    }

    ThrowIfFailed(swapChain.As(&m_swapChain));

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void RenderEngine::CreateMainDepthBuffer()
{
    m_depthBuffer.Initialize(m_width, m_height, Renderer::dsBufferFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                             D3D12_RESOURCE_STATE_DEPTH_WRITE, 0, L"mainDepthBuffer");

    m_dsvHeap.CreateResourceView(m_depthBuffer.Get(), DescriptorType::DSV);
}

void RenderEngine::CreateDepthBuffers()
{
    CreateMainDepthBuffer();
}

void RenderEngine::CreateDescriptorHeaps()
{
    // DescriptorHeap 생성
    m_swapChainRTVHeap.Initialize(m_swapChainBufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 0,
                                  D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    m_dsvHeap.Initialize(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
}

// BOOKMARK
void RenderEngine::UpdateGUI()
{
    // ImGui::SetWindowSize(ImVec2((float)m_guiWidth, (float)m_height));
    ImGui::SetWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);

    if (!m_world)
        return;

    // 신 전환 콤보 — 실제 전환은 다음 프레임 경계(Tick 첫머리)에서 적용된다
    Scene* active = m_world->GetActiveScene();
    const auto& names = m_world->GetSceneNames();
    if (!names.empty())
    {
        int current = 0;
        std::vector<const char*> items;
        items.reserve(names.size());
        for (int i = 0; i < (int)names.size(); i++)
        {
            items.push_back(names[i].c_str());
            if (active && names[i] == active->GetName())
                current = i;
        }
        if (ImGui::Combo("Scene", &current, items.data(), (int)items.size()))
            m_world->RequestScene(names[current]);
    }

    // 신별 컨트롤 (PBR material 슬라이더 등)
    if (active)
        active->UpdateGUI();
}

// BOOKMARK
void RenderEngine::Tick(float deltaTime)
{
    // 신 전환은 프레임 경계에서만 적용 (Execute가 매 프레임 flush하므로 이 시점 GPU는 idle)
    m_world->ApplyPendingScene(*this);

    Scene* scene = m_world->GetActiveScene();
    if (!scene)
        return;

    auto inputHelper = m_world->m_inputHelper;
    if (inputHelper && inputHelper->GetCaptureFlag())
    {
        inputHelper->SetCaptureFlag(false);
        std::string path = "Results/" + utility->MakeTimestamp() + ".png";
        SaveTextureGPU(path);
    }
    if (inputHelper && inputHelper->GetRecordFlag())
    {
        m_recording = true;
        inputHelper->SetRecordFlag(false);
    }
    // 숫자키 1~4 신 전환 (GUI 콤보와 동일하게 pending → 다음 프레임 경계에서 적용)
    if (inputHelper)
    {
        const auto& names = m_world->GetSceneNames();
        for (int i = 0; i < (int)names.size() && i < 9; i++)
        {
            if (inputHelper->GetKeyState('1' + i))
                m_world->RequestScene(names[i]);
        }
    }

    if (m_recording)
		Record(deltaTime);

	// 렌더링 시에 사용되는 constant buffer 갱신 ( 애니메이션, material, local, 등 )
    auto& renderScene = scene->GetRenderScene();
    for (auto& m : renderScene.meshBatchs)
    {
        m->SyncCB(deltaTime);
    }

    // OnRegister 단계에서 저장된 CameraComponent들의 constantbuffer 갱신
    for (auto& camera : renderScene.cameras)
    {
        camera->SyncCB();
    }

    // 시뮬레이션 커맨드 기록 + 제출 (신에 시뮬이 없으면 no-op)
    scene->Simulate(deltaTime, m_commandQueue.Get());

    ResetCommand();
    scene->Render(*this);
    RenderGUI();
    Execute();
}

void RenderEngine::GenerateNoise()
{
    m_noise->ResetCommand();
    m_noise->GeneratePerlinNoise();
    m_noise->GenerateCurlNoise();
    m_noise->Execute(m_commandQueue.Get());
}

void RenderEngine::InitRenderPipeline(bool clear)
{
    m_commandList->RSSetScissorRects(1, &m_scissorRect);
    m_commandList->RSSetViewports(1, &m_viewport);

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    if (clear)
    {
        m_commandList->ClearRenderTargetView(GetCurrentRtvCpuHandle(), rtvClearColor.data(), 0, nullptr);
        m_commandList->ClearDepthStencilView(GetDSVCpuHandle(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f,
                                             0, 0, nullptr);
    }

    m_commandList->OMSetRenderTargets(1, &GetCurrentRtvCpuHandle(), TRUE, &GetDSVCpuHandle(0));
}

void RenderEngine::RenderMeshes()
{
    using namespace Renderer;

    Scene* scene = m_world->GetActiveScene();
    if (!scene)
        return;

    InitRenderPipeline();
    bool heapBinded = false;

    auto player = m_world->GetPlayer();
    auto camera = player ? player->GetCameraComponent() : nullptr;
    auto ibl = m_world->GetIBL();
    auto light = m_world->GetLightManger();

    for (auto& m : scene->GetRenderScene().meshBatchs)
    {
        bool psoChanged = m->InitGraphicsCommand(m_commandList.Get());

        if (!heapBinded)
        {
            heapBinded = true;
            BindMainHeap();
        }
        if (psoChanged)
        {
            auto rs = m->GetCurrentRootSignature();

            if (camera)
                camera->Bind(rs, m_commandList.Get());
            if (ibl)
                ibl->Bind(rs, m_commandList.Get());
            if (light)
                light->Bind(rs, m_commandList.Get());
        }

        m->Render(m_commandList.Get());
    }
    // 렌더링 후 pso name 초기화
    SetCurrPSOName("");
}

void RenderEngine::RenderSPH(const std::string& psoName, bool clear)
{
    using namespace Renderer;

    m_commandList->RSSetScissorRects(1, &m_scissorRect);
    m_commandList->RSSetViewports(1, &m_viewport);

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    if (clear)
    {
        m_commandList->ClearRenderTargetView(GetCurrentRtvCpuHandle(), rtvClearColor.data(), 0, nullptr);
        m_commandList->ClearDepthStencilView(GetDSVCpuHandle(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f,
                                             0, 0, nullptr);
    }
    m_commandList->OMSetRenderTargets(1, &GetCurrentRtvCpuHandle(), TRUE, &GetDSVCpuHandle(0));

    // SPH::Render는 PSO/RootSignature가 세팅된 커맨드 리스트를 전제한다 (MeshBatch와 동일 패턴)
    GraphicsPSO pso;
    if (!GetGraphicsPSO(psoName, pso))
    {
        std::cout << "RenderSPH: failed to find pso " << psoName << "\n";
        return;
    }
    m_commandList->SetPipelineState(pso.GetPSO());
    m_commandList->SetGraphicsRootSignature(pso.GetRootSignature()->GetSignature());

    // 파티클 셰이더가 g_globalConstant(b0)를 읽으므로 카메라 CB 바인딩 필수 (미바인딩 시 device removed)
    auto player = m_world->GetPlayer();
    auto camera = player ? player->GetCameraComponent() : nullptr;
    if (camera)
        camera->Bind(pso.GetRootSignature(), m_commandList.Get());

    m_sph->Render(m_commandList.Get());
}

void RenderEngine::CreateFluidTargets()
{
    const int w = m_width, h = m_height;
    const auto RT = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    const auto SR = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    m_ssfScene.Initialize(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, RT, SR, 1, L"ssfScene");
    m_ssfDepthRaw.Initialize(w, h, DXGI_FORMAT_R32_FLOAT, RT, SR, 1, L"ssfDepthRaw");
    m_ssfDepthA.Initialize(w, h, DXGI_FORMAT_R32_FLOAT, RT, SR, 1, L"ssfDepthA");
    m_ssfDepthB.Initialize(w, h, DXGI_FORMAT_R32_FLOAT, RT, SR, 1, L"ssfDepthB");
    m_ssfThick.Initialize(w, h, DXGI_FORMAT_R16_FLOAT, RT, SR, 1, L"ssfThick");
    m_ssfThickTmp.Initialize(w, h, DXGI_FORMAT_R16_FLOAT, RT, SR, 1, L"ssfThickTmp");
    m_ssfD32.Initialize(w, h, DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                        D3D12_RESOURCE_STATE_DEPTH_WRITE, 0, L"ssfD32");

    if (!m_ssfInit)
    {
        m_ssfRtvHeap.Initialize(6, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 0, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        m_ssfDsvHeap.Initialize(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        m_ssfSrvHeap.Initialize(6, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_ssfInit = true;
    }
    else
    {
        m_ssfRtvHeap.ResetIndex();
        m_ssfDsvHeap.ResetIndex();
        m_ssfSrvHeap.ResetIndex();
    }

    // RTV slots 0..5
    m_ssfRtvHeap.CreateResourceView(m_ssfScene.Get(), DescriptorType::RTV);
    m_ssfRtvHeap.CreateResourceView(m_ssfDepthRaw.Get(), DescriptorType::RTV);
    m_ssfRtvHeap.CreateResourceView(m_ssfDepthA.Get(), DescriptorType::RTV);
    m_ssfRtvHeap.CreateResourceView(m_ssfDepthB.Get(), DescriptorType::RTV);
    m_ssfRtvHeap.CreateResourceView(m_ssfThick.Get(), DescriptorType::RTV);
    m_ssfRtvHeap.CreateResourceView(m_ssfThickTmp.Get(), DescriptorType::RTV);
    m_ssfDsvHeap.CreateResourceView(m_ssfD32.Get(), DescriptorType::DSV);
    // SRV slots 0..5 (SAME order -> slot indices match the RTVs)
    m_ssfSrvHeap.CreateResourceView(m_ssfScene.Get(), DescriptorType::SRV);
    m_ssfSrvHeap.CreateResourceView(m_ssfDepthRaw.Get(), DescriptorType::SRV);
    m_ssfSrvHeap.CreateResourceView(m_ssfDepthA.Get(), DescriptorType::SRV);
    m_ssfSrvHeap.CreateResourceView(m_ssfDepthB.Get(), DescriptorType::SRV);
    m_ssfSrvHeap.CreateResourceView(m_ssfThick.Get(), DescriptorType::SRV);
    m_ssfSrvHeap.CreateResourceView(m_ssfThickTmp.Get(), DescriptorType::SRV);
}

// Screen-space fluid: background -> depth -> narrow-range smooth -> thickness ->
// composite -> (mode 1) debug particles. All passes share g_FLIP_RS + m_ssfSrvHeap.
// SRV slots: 0 scene, 1 depthRaw, 2 depthA, 3 depthB, 4 thick, 5 thickTmp.
void RenderEngine::RenderFluid(int viewMode)
{
    using namespace Renderer;
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    auto player = m_world->GetPlayer();
    auto camera = player ? player->GetCameraComponent() : nullptr;
    if (!camera || !m_flipWater || !m_ssfInit)
        return;

    auto* cl = m_commandList.Get();

    // --- per-frame SSF render constant from the camera ---
    Matrix vStored = camera->GetViewMatrix();   // engine stores matrices transposed
    Matrix pStored = camera->GetProjMatrix();
    Matrix vAct = vStored.Transpose();
    Matrix pAct = pStored.Transpose();
    Matrix invVP = (vAct * pAct).Invert();

    SceneComponent* root = player->GetRootComponent();
    Vector3 fwd = root->GetFrontDirection();   fwd.Normalize();
    Vector3 right = root->GetRightDirection(); right.Normalize();
    Vector3 up = fwd.Cross(right);             up.Normalize();
    Vector3 pos = root->GetLocation();

    FlipFrameConstant fc = {};
    fc.view = vStored;
    fc.proj = pStored;
    fc.invViewProj = invVP.Transpose();
    fc.projA = XMFLOAT4(pAct._11, pAct._22, pAct._33, pAct._43);
    fc.camFwd = XMFLOAT4(fwd.x, fwd.y, fwd.z, 0.f);
    fc.camRight = XMFLOAT4(right.x, right.y, right.z, 0.f);
    fc.camUp = XMFLOAT4(up.x, up.y, up.z, 0.f);
    fc.camPos = XMFLOAT4(pos.x, pos.y, pos.z, 0.f);
    fc.sunDir = XMFLOAT4(-0.5f, 0.8f, -0.25f, 0.f);
    fc.screen = XMFLOAT4((float)m_width, (float)m_height, 1.f / m_width, 1.f / m_height);
    fc.fluidA = XMFLOAT4(0.016f, 1.0f, 0.05f, 100.0f); // radius, thickScale, zNear, zFar
    fc.fluidB = XMFLOAT4(0.055f, 0.085f, 30.0f, 0.0f); // filterRadius, depthThresh, maxFilterPx
    fc.absorb = XMFLOAT4(6.0f, 1.4f, 0.8f, 0.08f);
    m_flipWater->SetFrame(fc);

    const UINT n = (UINT)m_flipWater->ParticleCount();

    auto psoOf = [&](const char* name) -> ID3D12PipelineState* {
        GraphicsPSO p;
        GetGraphicsPSO(name, p);
        return p.GetPSO();
    };
    auto barrierTo = [&](Texture2D& t, D3D12_RESOURCE_STATES s) {
        D3D12_RESOURCE_BARRIER b;
        if (t.Transition(s, b)) cl->ResourceBarrier(1, &b);
    };
    auto setPass = [&](UINT x, UINT y) { UINT v[4] = {x, y, 0, 0}; cl->SetGraphicsRoot32BitConstants(1, 4, v, 0); };
    auto table = [&](UINT param, int slot) { cl->SetGraphicsRootDescriptorTable(param, m_ssfSrvHeap.GetGPUHandle(slot)); };

    cl->RSSetScissorRects(1, &m_scissorRect);
    cl->RSSetViewports(1, &m_viewport);
    cl->SetGraphicsRootSignature(g_FLIP_RS.GetSignature());
    ID3D12DescriptorHeap* heaps[] = {m_ssfSrvHeap.GetHeap()};
    cl->SetDescriptorHeaps(1, heaps);
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl->SetGraphicsRootConstantBufferView(0, m_flipWater->FrameCBVA());     // b0 frame
    cl->SetGraphicsRootShaderResourceView(2, m_flipWater->ParticleVA());    // t4 particles (VS)

    const float farClear[4] = {fc.fluidA.w, 0, 0, 0};   // depthRaw background = zFar
    const float zeroClear[4] = {0, 0, 0, 0};

    // (1) background pool tank -> sceneColor (rgb linear + view depth in alpha)
    barrierTo(m_ssfScene, D3D12_RESOURCE_STATE_RENDER_TARGET);
    {
        auto rtv = m_ssfRtvHeap.GetCPUHandle(0);
        cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        setPass(0, 0);
        cl->SetPipelineState(psoOf("flipBgPSO"));
        cl->DrawInstanced(3, 1, 0, 0);
    }

    // (2) particle sphere depth -> depthRaw (R32F) + D32 (nearest)
    barrierTo(m_ssfDepthRaw, D3D12_RESOURCE_STATE_RENDER_TARGET);
    {
        auto rtv = m_ssfRtvHeap.GetCPUHandle(1);
        auto dsv = m_ssfDsvHeap.GetCPUHandle(0);
        cl->ClearRenderTargetView(rtv, farClear, 0, nullptr);
        cl->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
        cl->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        setPass(0, 0);
        cl->SetPipelineState(psoOf("flipDepthPSO"));
        cl->DrawInstanced(6, n, 0, 0);
    }
    barrierTo(m_ssfScene, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); // needed by thick + composite

    // (3) narrow-range depth smooth x N  (X -> depthA, Y -> depthB)
    const int kSmoothIters = 4;
    for (int it = 0; it < kSmoothIters; it++)
    {
        Texture2D& src = (it == 0) ? m_ssfDepthRaw : m_ssfDepthB;
        int srcSlot = (it == 0) ? 1 : 3;
        barrierTo(src, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        barrierTo(m_ssfDepthA, D3D12_RESOURCE_STATE_RENDER_TARGET);
        {
            auto rtv = m_ssfRtvHeap.GetCPUHandle(2);
            cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            setPass(0, 0);                     // X, narrow-range
            table(3, srcSlot);
            cl->SetPipelineState(psoOf("flipSmoothPSO"));
            cl->DrawInstanced(3, 1, 0, 0);
        }
        barrierTo(m_ssfDepthA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        barrierTo(m_ssfDepthB, D3D12_RESOURCE_STATE_RENDER_TARGET);
        {
            auto rtv = m_ssfRtvHeap.GetCPUHandle(3);
            cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            setPass(1, 0);                     // Y, narrow-range
            table(3, 2);                       // depthA
            cl->SetPipelineState(psoOf("flipSmoothPSO"));
            cl->DrawInstanced(3, 1, 0, 0);
        }
    }

    // (4) thickness (additive) -> thickness, occlusion via sceneColor.a ; then gaussian blur
    barrierTo(m_ssfThick, D3D12_RESOURCE_STATE_RENDER_TARGET);
    {
        auto rtv = m_ssfRtvHeap.GetCPUHandle(4);
        cl->ClearRenderTargetView(rtv, zeroClear, 0, nullptr);
        cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        setPass(0, 0);
        table(3, 0);                           // sceneColor -> t0 (occlusion)
        cl->SetPipelineState(psoOf("flipThickPSO"));
        cl->DrawInstanced(6, n, 0, 0);
    }
    barrierTo(m_ssfThick, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barrierTo(m_ssfThickTmp, D3D12_RESOURCE_STATE_RENDER_TARGET);
    {
        auto rtv = m_ssfRtvHeap.GetCPUHandle(5);
        cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        setPass(0, 1);                         // X, gaussian
        table(3, 4);                           // thick -> t0
        cl->SetPipelineState(psoOf("flipBlurPSO"));
        cl->DrawInstanced(3, 1, 0, 0);
    }
    barrierTo(m_ssfThickTmp, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barrierTo(m_ssfThick, D3D12_RESOURCE_STATE_RENDER_TARGET);
    {
        auto rtv = m_ssfRtvHeap.GetCPUHandle(4);
        cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        setPass(1, 1);                         // Y, gaussian
        table(3, 5);                           // thickTmp -> t0
        cl->SetPipelineState(psoOf("flipBlurPSO"));
        cl->DrawInstanced(3, 1, 0, 0);
    }

    // (5) composite -> backbuffer
    barrierTo(m_ssfDepthRaw, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barrierTo(m_ssfDepthB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barrierTo(m_ssfThick, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    {
        std::vector<D3D12_RESOURCE_BARRIER> bs;
        D3D12_RESOURCE_BARRIER b;
        if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, b))
            bs.push_back(b);
        if (!bs.empty()) cl->ResourceBarrier((UINT)bs.size(), bs.data());
    }
    {
        auto bbrtv = GetCurrentRtvCpuHandle();
        cl->OMSetRenderTargets(1, &bbrtv, FALSE, nullptr);
        int fdSlot = (viewMode == 2) ? 1 : 3;  // raw depth : smoothed depth
        setPass((UINT)viewMode, 0);
        table(3, fdSlot);                      // depth -> t0
        table(4, 4);                           // thickness -> t1
        table(5, 0);                           // sceneColor -> t2
        cl->SetPipelineState(psoOf("flipCompositePSO"));
        cl->DrawInstanced(3, 1, 0, 0);
    }

    // (6) view mode 1: velocity-colored debug spheres over the tank (D32 EQUAL)
    if (viewMode == 1)
    {
        auto bbrtv = GetCurrentRtvCpuHandle();
        auto dsv = m_ssfDsvHeap.GetCPUHandle(0);
        cl->OMSetRenderTargets(1, &bbrtv, FALSE, &dsv);
        setPass(0, 0);
        cl->SetPipelineState(psoOf("flipParticleRenderPSO"));
        cl->DrawInstanced(6, n, 0, 0);
    }
}

void RenderEngine::RenderGUI()
{
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags flags = 0;

    ImGui::Begin("GUI", nullptr, flags);
    UpdateGUI();

    ImGui::End();
    ImGui::Render();

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    m_commandList->OMSetRenderTargets(1, &GetCurrentRtvCpuHandle(), TRUE, &GetDSVCpuHandle(0));
    ID3D12DescriptorHeap* pHeaps[] = {m_guiFontHeap.Get()};
    m_commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(pHeaps)), pHeaps);

    ImDrawData* dd = ImGui::GetDrawData();
    if (dd->Valid)
        ImGui_ImplDX12_RenderDrawData(dd, m_commandList.Get());
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderEngine::GetCurrentRtvCpuHandle() const
{
    return m_swapChainRTVHeap.GetCPUHandle(m_currentBackBufferIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderEngine::GetDSVCpuHandle(int idx) const
{
    return m_dsvHeap.GetCPUHandle(idx);
}

ID3D12Resource* RenderEngine::GetCurrentSwapChainResource() const
{
    return m_swapChainResources[m_currentBackBufferIndex].Get();
}

void RenderEngine::ResetCommand()
{
    m_commandAllocator->Reset();
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}

void RenderEngine::BindMainHeap()
{
    ID3D12DescriptorHeap* heaps[] = {m_world->GetMainHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
}

void RenderEngine::FlushCommands()
{
    m_currentBufferFence++;

    ThrowIfFailed(m_commandQueue->Signal(m_createBufferfence.Get(), m_currentBufferFence));

    if (m_createBufferfence->GetCompletedValue() < m_currentBufferFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        m_createBufferfence->SetEventOnCompletion(m_currentBufferFence, eventHandle);

        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void RenderEngine::FlushRenderCommands()
{
    m_currentFence++;

    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));
    if (m_fence->GetCompletedValue() < m_currentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        m_fence->SetEventOnCompletion(m_currentFence, eventHandle);

        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void RenderEngine::Quit()
{
    FlushRenderCommands();
}

void RenderEngine::GenerateMips(ID3D12Resource* tex)
{
    DirectX::ResourceUploadBatch upload(m_device);
    upload.Begin(D3D12_COMMAND_LIST_TYPE_DIRECT);

    upload.GenerateMips(tex);

    auto finish = upload.End(m_commandQueue.Get());
    finish.wait();
}

void RenderEngine::Execute()
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_PRESENT, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    m_commandList->Close();
    ID3D12CommandList* commands[] = {m_commandList.Get()};
    m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);

    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));
    ThrowIfFailed(m_swapChain->Present(1, 0));
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    FlushRenderCommands();
}

ImageInfo RenderEngine::ReadbackBackbuffer()
{
    ImageInfo info;
    UINT subresource = 0;
    info.numRows = 0;
    info.rowSize = 0;
    UINT64 requiredSize = 0;

    auto t = GetCurrentSwapChainResource();
    D3D12_RESOURCE_DESC desc = t->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    m_device->GetCopyableFootprints(&desc, subresource, 1, 0, &footprint, &info.numRows, &info.rowSize, &requiredSize);

    info.width = footprint.Footprint.Width;
    info.height = footprint.Footprint.Height;
    info.rowPitch = footprint.Footprint.RowPitch;

    FlushCommands();
    m_resourceCommandAllocator->Reset();
    ThrowIfFailed(m_resourceCommandList->Reset(m_resourceCommandAllocator.Get(), nullptr));

    m_world->InitReadbackBuffer(requiredSize);
    auto readbackBuffer = m_world->GetReadBackBuffer();

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, barrier))
        barriers.push_back(barrier);

    if (!barriers.empty())
        m_resourceCommandList->ResourceBarrier(barriers.size(), barriers.data());

    CD3DX12_TEXTURE_COPY_LOCATION dst(readbackBuffer->Get(), footprint);
    D3D12_TEXTURE_COPY_LOCATION src{};
    src.pResource = t;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = subresource;

    m_resourceCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    std::vector<D3D12_RESOURCE_BARRIER> barriers2;

    if (m_swapChainResources[m_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_PRESENT, barrier))
    {
        barriers2.push_back(barrier);
    }
    if (!barriers2.empty())
    {
        m_resourceCommandList->ResourceBarrier(barriers2.size(), barriers2.data());
    }

    m_resourceCommandList->Close();
    ID3D12CommandList* commands[] = {m_resourceCommandList.Get()};
    m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);
    FlushCommands();

    return info;
}

void RenderEngine::SaveTextureGPU(const std::string& resultPath)
{
    ImageInfo info = ReadbackBackbuffer();
    info.resultPath = resultPath;
    m_world->Notify(info);
}

// ffmpeg를 자식 프로세스로 띄우고 stdin 파이프로 raw RGBA 프레임을 흘려보낸다.
// 백버퍼는 R16G16B16A16_FLOAT → ReadbackToRGBA에서 8bit RGBA(톤매핑)로 변환된 바이트를 그대로 사용.
bool RenderEngine::OpenVideoPipe(int width, int height, int inFps, int outFps, const std::string& outPath)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE; // 자식이 read end를 상속받도록
    sa.lpSecurityDescriptor = nullptr;

    HANDLE childStdinRd = nullptr;
    HANDLE childStdinWr = nullptr;
    if (!CreatePipe(&childStdinRd, &childStdinWr, &sa, 0))
    {
        std::cout << "OpenVideoPipe: CreatePipe failed\n";
        return false;
    }
    // 부모가 쓰는 write end는 자식이 상속하지 않게 한다.
    SetHandleInformation(childStdinWr, HANDLE_FLAG_INHERIT, 0);

    const char* ffmpegEnv = std::getenv("SENGINE_FFMPEG");
    std::string ffmpeg = ffmpegEnv ? ffmpegEnv : "C:\\ffmpeg\\bin\\ffmpeg.exe";

    // 저장 fps가 입력과 다르면 출력 -r 옵션을 붙인다 (프레임 복제/드롭, 길이 유지)
    char rOpt[24] = "";
    if (outFps > 0 && outFps != inFps)
        std::snprintf(rOpt, sizeof(rOpt), "-r %d ", outFps);

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd),
                  "\"%s\" -y -f rawvideo -pixel_format rgba -video_size %dx%d -framerate %d -i - "
                  "%s-c:v libx264 -pix_fmt yuv420p -crf 18 -movflags +faststart \"%s\"",
                  ffmpeg.c_str(), width, height, inFps, rOpt, outPath.c_str());

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = childStdinRd;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, cmd, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    CloseHandle(childStdinRd); // read end는 자식 소유 → 부모는 닫는다
    if (!ok)
    {
        std::cout << "OpenVideoPipe: CreateProcess failed (ffmpeg 경로 확인: " << ffmpeg << ")\n";
        CloseHandle(childStdinWr);
        return false;
    }
    CloseHandle(pi.hThread);
    m_ffmpegProcess = pi.hProcess;
    m_ffmpegStdin = childStdinWr;
    m_videoPipeOpen = true;
    std::cout << "OpenVideoPipe: " << cmd << std::endl;
    return true;
}

void RenderEngine::WriteVideoFrame(const std::vector<uint8_t>& frame)
{
    if (!m_videoPipeOpen || frame.empty())
        return;

    const uint8_t* p = frame.data();
    size_t remaining = frame.size();
    while (remaining > 0)
    {
        DWORD toWrite = (remaining > (1u << 20)) ? (1u << 20) : (DWORD)remaining;
        DWORD written = 0;
        if (!WriteFile(m_ffmpegStdin, p, toWrite, &written, nullptr) || written == 0)
        {
            std::cout << "WriteVideoFrame: pipe write 실패 (ffmpeg 종료?)\n";
            m_videoPipeOpen = false;
            return;
        }
        p += written;
        remaining -= written;
    }
}

void RenderEngine::CloseVideoPipe()
{
    if (m_ffmpegStdin)
    {
        CloseHandle(m_ffmpegStdin); // stdin EOF → ffmpeg가 인코딩을 마무리하고 mp4 trailer 기록
        m_ffmpegStdin = nullptr;
    }
    if (m_ffmpegProcess)
    {
        WaitForSingleObject(m_ffmpegProcess, INFINITE);
        CloseHandle(m_ffmpegProcess);
        m_ffmpegProcess = nullptr;
    }
    m_videoPipeOpen = false;
}

void RenderEngine::RegistMeshBatch(std::shared_ptr<MeshBatch> meshBatch)
{
    // Scene::Load 중에만 호출된다 — 현재 로딩 중인 신의 RenderScene에 쌓는다.
    // Material이 nullptr이면 MeshBatch::Render에서 early-return. fallback은 호출측이 책임진다.
    if (m_registrationTarget)
        m_registrationTarget->meshBatchs.push_back(std::move(meshBatch));
}

void RenderEngine::RegistCamera(CameraComponent* camera)
{
    if (m_registrationTarget)
        m_registrationTarget->cameras.push_back(camera);
}

// === 녹화 기능
void RenderEngine::Record(float& deltaTime)
{
    deltaTime = m_recordDt; // 고정 timestep (벽시계 dt 무시 → 매끄러운 영상)
    if (m_recordFrame >= m_recordWarmup)
    {
        int idx = m_recordFrame - m_recordWarmup;
        if (idx < m_recordCount)
        {
            ImageInfo info = ReadbackBackbuffer();
            if (idx == 0)
            {
                int inFps = (int)(1.0f / m_recordDt + 0.5f);
                const char* outEnv = std::getenv("SENGINE_RECORD_OUT");
                std::string outPath = outEnv ? outEnv : "Results/Videos/" + videoName + ".mp4";
                OpenVideoPipe((int)info.width, (int)info.height, inFps, m_outputFps, outPath);
            }
            std::vector<uint8_t> rgba;
            m_world->ReadbackToRGBA(info, rgba);
            WriteVideoFrame(rgba);
        }
        else
        {
            CloseVideoPipe();
            m_recording = false;
            std::cout << "RECORD_DONE frames=" << m_recordCount << std::endl;
            // PostQuitMessage(0);
        }
    }
    m_recordFrame++;
}
