#include "Noise.h"
#include "GraphicsCommon.h"
#include "Engine\world.h"
#include "Renderer.h"
#include <random>

Noise::Noise()
{
}

Noise::~Noise()
{
}
void Noise::Initialize(UINT width, UINT height, ID3D12GraphicsCommandList* cmdList)
{
    InitCommands();
    InitCPU();
    InitGPU(width, height, cmdList);
}

void Noise::InitCommands()
{
    if (m_world && m_world->GetDevice())
    {
        auto device = m_world->GetDevice();
        ThrowIfFailed(
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr,
                                                IID_PPV_ARGS(&m_commandList)));
        m_commandList->Close();
    }
}
void Noise::InitCPU()
{
    particleCPU.resize(particleCount);
   
	std::random_device rd;
    std::mt19937 gen(rd());
    float PI = 3.141592f;

    std::uniform_real_distribution<float> randomLen(0.1f, 1.f);
    std::uniform_real_distribution<float> randomTheta(-PI, PI);
    std::uniform_real_distribution<float> randomSize(0.01f, 0.02f);
    std::uniform_real_distribution<float> randomColor(0.2f, 1.f);
    for (size_t i = 0; i < particleCount; i++)
    {
        float theta = randomTheta(gen);
        float l = randomLen(gen);
        particleCPU[i].position = l* Vector3(std::cos(theta), std::sin(theta), 0.f);
        particleCPU[i].radius = randomSize(gen);
        particleCPU[i].color = Vector3(randomColor(gen), randomColor(gen), randomColor(gen));
    }
    // std::shared_ptr<StaticMesh> mesh = std::make_shared<StaticMesh>();
    // mesh->InitializePoints(particleCount);
    // m_world->AddMesh("NoiseParticles", mesh);
}

void Noise::InitGPU(UINT width, UINT height, ID3D12GraphicsCommandList* cmdList)
{
    m_width = width;
    m_height = height;

    m_noiseHeap.Initialize(3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    D3D12_RESOURCE_FLAGS allowUAflag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    m_perlinNoise.Initialize(perlinWidth, perlinHeight, perlinFormat, allowUAflag,
                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, L"perlinNoise");
    m_curlNoise.Initialize(m_width, m_height, curlFormat, allowUAflag, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0,
                           L"curlNoise");
    m_density.Initialize(m_width, m_height, densityFormat, allowUAflag, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0,
                         L"noiseDensity");

    particles.Initialize(particleCPU, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, cmdList, L"NoiseParticle");

    NoiseLocalConstant init;
    init.deltaTime = 0;
    init.particleCount = particleCount;
    init.grid = Vector2((float)m_width, (float)m_height);
    m_noiseLCB.Initialize(init);
    m_noiseLCB.Update();

    // descriptor view 생성
    m_noiseHeap.CreateResourceView(m_perlinNoise.Get(), DescriptorType::UAV, ViewDimensionType::TEXTURE2D);
    m_noiseHeap.CreateResourceView(m_curlNoise.Get(), DescriptorType::UAV, ViewDimensionType::TEXTURE2D);
    m_noiseHeap.CreateResourceView(m_density.Get(), DescriptorType::UAV, ViewDimensionType::TEXTURE2D);

    if (m_world->GetTextureLoader())
    {
        m_world->AddTexture("noiseDensity", m_density);
        m_world->AddTexture("perlinNoise", m_perlinNoise);
    }
}

void Noise::GeneratePerlinNoise()
{
    // resource barrier
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_perlinNoise.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    // bind PSO
    Renderer::BindCPSO(perlinCPSOName, m_commandList.Get());

    // bind heap
    m_noiseHeap.Bind(m_commandList.Get());

    m_commandList->SetComputeRootDescriptorTable(0, m_noiseHeap.GetGPUHandle(0));

    Dispatch(perlinWidth, perlinHeight, N_GROUP_SIZE_X, N_GROUP_SIZE_Y);
}

void Noise::GenerateCurlNoise()
{
    // resource barrier
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_curlNoise.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (m_perlinNoise.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    // bind PSO
    Renderer::BindCPSO(curlCPSOName, m_commandList.Get());

    // bind heap
    m_noiseHeap.Bind(m_commandList.Get());

    // 0 : perlinNoise, 1 : curlNoise
    m_commandList->SetComputeRootDescriptorTable(0, m_noiseHeap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, m_noiseHeap.GetGPUHandle(1));

    Dispatch(m_width, m_height, N_GROUP_SIZE_X, N_GROUP_SIZE_Y);
}

void Noise::CurlNoiseSimulation(float deltaTime)
{
    // update consant buffer
    m_noiseLCB.localConstant.deltaTime = deltaTime;
    m_noiseLCB.Update();

    Sourcing();
    ParticleAdvection();
}

void Noise::Sourcing()
{
    // resource barrier
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_density.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (particles.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    // bind PSO
    Renderer::BindCPSO(noiseSourcingCPSOName, m_commandList.Get());

    // bind heap
    m_noiseHeap.Bind(m_commandList.Get());

    // 0 : particle, 1 : density, 2 : lcb
    m_commandList->SetComputeRootShaderResourceView(0, particles.GetGPUVirtualAddress());
    m_commandList->SetComputeRootDescriptorTable(1, m_noiseHeap.GetGPUHandle(2));
    m_commandList->SetComputeRootConstantBufferView(2, m_noiseLCB.GetGPUAddress());

    Dispatch(m_width, m_height, N_GROUP_SIZE_X, N_GROUP_SIZE_Y);
}

void Noise::ParticleAdvection()
{
    // resource barrier
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_curlNoise.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (particles.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    // bind PSO
    Renderer::BindCPSO(curlSimulationCPSOName, m_commandList.Get());

    // bind heap
    m_noiseHeap.Bind(m_commandList.Get());

    // 0 : particle, 1 : lcb
    m_commandList->SetComputeRootUnorderedAccessView(0, particles.GetGPUVirtualAddress());
    m_commandList->SetComputeRootConstantBufferView(1, m_noiseLCB.GetGPUAddress());

    Dispatch(particleCount, SIMULATION_GROUP_SIZE_X);
}

void Noise::RenderParticles(ID3D12GraphicsCommandList* c)
{
    // resource barrier
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (particles.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (!barriers.empty())
        c->ResourceBarrier((UINT)barriers.size(), barriers.data());

    c->SetGraphicsRootShaderResourceView(0, particles.GetGPUVirtualAddress());
    c->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    c->DrawInstanced(particleCount, 1, 0, 0);
}

void Noise::Execute(ID3D12CommandQueue* commandQueue)
{
    m_commandList->Close();
    ID3D12CommandList* commands[] = {m_commandList.Get()};
    commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);
}

void Noise::Dispatch(UINT width, UINT height, UINT gs_x, UINT gs_y)
{
    UINT x = (width + gs_x - 1) / gs_x;
    UINT y = (height + gs_y - 1) / gs_y;
    m_commandList->Dispatch(x, y, 1);
}

void Noise::Dispatch(UINT count, UINT gs_x)
{
    UINT x = (count + gs_x - 1) / gs_x;
    m_commandList->Dispatch(x, 1, 1);
}

void Noise::SetCPSO(const std::string psoName)
{
    Renderer::BindCPSO(psoName, m_commandList.Get());
}

void Noise::ResetCommand()
{
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);
}
