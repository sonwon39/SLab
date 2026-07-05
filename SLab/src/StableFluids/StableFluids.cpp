#include "StableFluids.h"
#include "GraphicsCommon.h"
#include "Engine/World.h"
#include "Renderer.h"

#include <vector>

using namespace Graphics;
using namespace std;
StableFluids::StableFluids()
{
}

void StableFluids::Initialize(UINT width, UINT height)
{
    InitCommands();
    InitResources(width, height);
}

void StableFluids::InitCommands()
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

void StableFluids::InitResources(UINT width, UINT height)
{
    gridWidth = width;
    gridHeight = height;

    // heap 초기화
    {
        m_renderDensityHeap.Initialize(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                       D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_sourcingHeap.Initialize(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_computeCurlHeap.Initialize(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                     D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_vorticityConfinementHeap.Initialize(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                              D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_advectionHeap.Initialize(4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_computeDivergenceHeap.Initialize(4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                           D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_jacobiHeap[0].Initialize(3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_jacobiHeap[1].Initialize(3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_computeFinalVelocityHeap.Initialize(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0,
                                              D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    }
    // 버퍼 초기화
    {
        D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        m_oldDensityBuffer.Initialize(gridWidth, gridHeight, densityFormat, flag, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                      0, L"oldDensity Buffer");
        m_oldVelocityBuffer.Initialize(gridWidth, gridHeight, velocityFormat, flag,
                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, L"oldVelocity Buffer");
        m_newDensityBuffer.Initialize(gridWidth, gridHeight, densityFormat, flag, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                      0, L"density Buffer");
        m_newVelocityBuffer.Initialize(gridWidth, gridHeight, velocityFormat, flag,
                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, L"velocity Buffer");
        m_divergenceBuffer.Initialize(gridWidth, gridHeight, divergenceFormat, flag,
                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, L"divergence Buffer");
        m_curlBuffer.Initialize(gridWidth, gridHeight, curlFormat, flag, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0,
                                L"divergence Buffer");
        for (size_t i = 0; i < 2; i++)
        {
            std::wstring name = L"pressure Buffer" + std::to_wstring(i);
            m_pressureBuffer[i].Initialize(gridWidth, gridHeight, pressureFormat, flag,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, name);
        }

        m_sourcingHeap.CreateResourceView(m_oldDensityBuffer.Get(), DescriptorType::UAV);
        m_sourcingHeap.CreateResourceView(m_oldVelocityBuffer.Get(), DescriptorType::UAV);

        m_computeCurlHeap.CreateResourceView(m_oldVelocityBuffer.Get(), DescriptorType::SRV);
        m_computeCurlHeap.CreateResourceView(m_curlBuffer.Get(), DescriptorType::UAV);

        m_vorticityConfinementHeap.CreateResourceView(m_curlBuffer.Get(), DescriptorType::SRV);
        m_vorticityConfinementHeap.CreateResourceView(m_oldVelocityBuffer.Get(), DescriptorType::UAV);

        m_advectionHeap.CreateResourceView(m_oldDensityBuffer.Get(), DescriptorType::SRV);
        m_advectionHeap.CreateResourceView(m_oldVelocityBuffer.Get(), DescriptorType::SRV);
        m_advectionHeap.CreateResourceView(m_newDensityBuffer.Get(), DescriptorType::UAV);
        m_advectionHeap.CreateResourceView(m_newVelocityBuffer.Get(), DescriptorType::UAV);

        m_computeDivergenceHeap.CreateResourceView(m_oldVelocityBuffer.Get(), DescriptorType::SRV);
        m_computeDivergenceHeap.CreateResourceView(m_divergenceBuffer.Get(), DescriptorType::UAV);
        m_computeDivergenceHeap.CreateResourceView(m_pressureBuffer[0].Get(), DescriptorType::UAV);
        m_computeDivergenceHeap.CreateResourceView(m_pressureBuffer[1].Get(), DescriptorType::UAV);

        m_jacobiHeap[0].CreateResourceView(m_divergenceBuffer.Get(), DescriptorType::SRV);
        m_jacobiHeap[0].CreateResourceView(m_pressureBuffer[0].Get(), DescriptorType::SRV);
        m_jacobiHeap[0].CreateResourceView(m_pressureBuffer[1].Get(), DescriptorType::UAV);

        m_jacobiHeap[1].CreateResourceView(m_divergenceBuffer.Get(), DescriptorType::SRV);
        m_jacobiHeap[1].CreateResourceView(m_pressureBuffer[1].Get(), DescriptorType::SRV);
        m_jacobiHeap[1].CreateResourceView(m_pressureBuffer[0].Get(), DescriptorType::UAV);

        m_computeFinalVelocityHeap.CreateResourceView(m_pressureBuffer[1].Get(), DescriptorType::SRV);
        m_computeFinalVelocityHeap.CreateResourceView(m_oldVelocityBuffer.Get(), DescriptorType::UAV);

        if (m_world)
        {
            m_world->AddTexture("sf_density", m_newDensityBuffer);
        }
    }

    SFLocalConstant grid;
    grid.gGridDim = Vector3((float)gridWidth, (float)gridHeight, 1.f);
    grid.h = 1.f;
    gridCB.Initialize(grid);
}

void StableFluids::Tick(float deltaTime)
{
    gridCB.localConstant.deltaTime = deltaTime;
    gridCB.Update();

    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    Sourcing();

    Projection();
    Advection();

    CopyDensityAndVelocity();

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;
    if (m_newDensityBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
}

void StableFluids::CopyDensityAndVelocity()
{
    vector<D3D12_RESOURCE_BARRIER> barriers0;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_oldDensityBuffer.Transition(D3D12_RESOURCE_STATE_COPY_DEST, barrier))
        barriers0.push_back(barrier);
    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_COPY_DEST, barrier))
        barriers0.push_back(barrier);
    if (m_newDensityBuffer.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, barrier))
        barriers0.push_back(barrier);
    if (m_newVelocityBuffer.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, barrier))
        barriers0.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers0.size(), barriers0.data());

    m_commandList->CopyResource(m_oldDensityBuffer.Get(), m_newDensityBuffer.Get());
    m_commandList->CopyResource(m_oldVelocityBuffer.Get(), m_newVelocityBuffer.Get());
}

void StableFluids::Sourcing()
{
    AddSource();
    AddVorticity();
}

void StableFluids::AddSource()
{
    SetCPSO("sourcingCPSO");

    DescriptorHeap& heap = m_sourcingHeap;
    vector<D3D12_RESOURCE_BARRIER> barriers0;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_oldDensityBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers0.push_back(barrier);
    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers0.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers0.size(), barriers0.data());

    ID3D12DescriptorHeap* heaps[] = {heap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, heap.GetGPUHandle(0));
    m_commandList->SetComputeRootConstantBufferView(1, gridCB.GetGPUAddress());
    m_commandList->SetComputeRootConstantBufferView(2, m_world->mouse->mouseCB.GetGPUAddress());

    Dispatch();
}

void StableFluids::AddVorticity()
{
    ComputeVelocityCurl();
    VorticityConfinement();
}

void StableFluids::ComputeVelocityCurl()
{
    SetCPSO("computeCurlCPSO");

    DescriptorHeap& heap = m_computeCurlHeap;

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_curlBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {heap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, heap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, heap.GetGPUHandle(1));
    m_commandList->SetComputeRootConstantBufferView(2, gridCB.GetGPUAddress());

    Dispatch();
}

void StableFluids::VorticityConfinement()
{
    SetCPSO("vorticityConfinementCPSO");

    DescriptorHeap& heap = m_vorticityConfinementHeap;

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_curlBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {heap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, heap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, heap.GetGPUHandle(1));
    m_commandList->SetComputeRootConstantBufferView(2, gridCB.GetGPUAddress());

    Dispatch();
}

void StableFluids::Advection()
{
    SetCPSO("advectionCPSO");

    DescriptorHeap& advectionHeap = m_advectionHeap;

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_oldDensityBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (m_newDensityBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (m_newVelocityBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {advectionHeap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, advectionHeap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, advectionHeap.GetGPUHandle(2));
    m_commandList->SetComputeRootConstantBufferView(2, gridCB.GetGPUAddress());

    Dispatch();
}

void StableFluids::Projection()
{
    ComputeDivergence();

    for (int i = 0; i < 100; i++)
    {
        Jacobi(i);
    }

    Finalize();
}

void StableFluids::ComputeDivergence()
{
    SetCPSO("computeDivergenceCPSO");

    DescriptorHeap& computeDivergenceHeap = m_computeDivergenceHeap;

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (m_divergenceBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {computeDivergenceHeap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, computeDivergenceHeap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, computeDivergenceHeap.GetGPUHandle(1));
    m_commandList->SetComputeRootConstantBufferView(2, gridCB.GetGPUAddress());

    Dispatch();
}

void StableFluids::Jacobi(int idx)
{
    SetCPSO("jacobiCPSO");

    int index0 = idx % 2;
    int index1 = (idx + 1) % 2;

    DescriptorHeap& jacobiHeap = m_jacobiHeap[index0];

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_pressureBuffer[index0].Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (m_pressureBuffer[index1].Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    if (m_divergenceBuffer.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);

    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {jacobiHeap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, jacobiHeap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, jacobiHeap.GetGPUHandle(2));
    m_commandList->SetComputeRootConstantBufferView(2, gridCB.GetGPUAddress());

    Dispatch();
    // CopyPressure();
}

void StableFluids::CopyPressure()
{

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_pressureBuffer[0].Transition(D3D12_RESOURCE_STATE_COPY_DEST, barrier))
        barriers.push_back(barrier);
    if (m_pressureBuffer[1].Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    m_commandList->CopyResource(m_pressureBuffer[0].Get(), m_pressureBuffer[1].Get());
}

void StableFluids::Finalize()
{
    SetCPSO("computeFinalVelocityCPSO");

    DescriptorHeap& heap = m_computeFinalVelocityHeap;

    vector<D3D12_RESOURCE_BARRIER> barriers;
    D3D12_RESOURCE_BARRIER barrier;

    if (m_pressureBuffer[1].Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, barrier))
        barriers.push_back(barrier);
    if (m_oldVelocityBuffer.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
        barriers.push_back(barrier);
    m_commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {heap.GetHeap()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootDescriptorTable(0, heap.GetGPUHandle(0));
    m_commandList->SetComputeRootDescriptorTable(1, heap.GetGPUHandle(1));
    m_commandList->SetComputeRootConstantBufferView(2, gridCB.GetGPUAddress());

    Dispatch();
}

void StableFluids::Execute(ID3D12CommandQueue* commandQueue)
{
    m_commandList->Close();
    ID3D12CommandList* commands[] = {m_commandList.Get()};
    commandQueue->ExecuteCommandLists(ARRAYSIZE(commands), commands);
}

void StableFluids::Dispatch()
{
    UINT width = (UINT)gridCB.localConstant.gGridDim.x;
    UINT height = (UINT)gridCB.localConstant.gGridDim.y;
    UINT groupCountX = (width + SF_GROUP_SIZE_X - 1) / SF_GROUP_SIZE_X;
    UINT groupCountY = (height + SF_GROUP_SIZE_Y - 1) / SF_GROUP_SIZE_Y;
    m_commandList->Dispatch(groupCountX, groupCountY, 1);
}

void StableFluids::SetCPSO(const std::string psoName)
{
    Renderer::BindCPSO(psoName, m_commandList.Get());
}
