#pragma once

#include <vector>
#include "directxtk12\SimpleMath.h"
#include "wrl.h"
#include "d3d12.h"
#include "PipelineState.h"
#include "Core/Texture2D.h"
#include "DescriptorHeap.h"
#include "Core/ConstantBuffer.h"
#include "Grid.h"

class RenderEngine;

class StableFluids
{
    friend class RenderEngine;

  public:
    StableFluids();

  public:
    void Initialize(UINT width, UINT height);
    void InitCommands();
    void InitResources(UINT width, UINT height);

  public:
    void Tick(float deltaTime);

    void CopyDensityAndVelocity();
    void Sourcing();
    void AddSource();
    void AddVorticity();
    void ComputeVelocityCurl();
    void VorticityConfinement();
    void Advection();
    void Projection();
    void ComputeDivergence();
    void Jacobi(int idx);
    void CopyPressure();
    void Finalize();
    void Execute(ID3D12CommandQueue* commandQueue);

    void Dispatch();

    void SetCPSO(const std::string psoName);

  private:
    UINT m_cbvSrvDescriptorSize = 0;
    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;

  private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

    // stable fluids
  protected:
    Texture2D m_oldDensityBuffer;
    Texture2D m_oldVelocityBuffer;

    Texture2D m_newDensityBuffer;
    Texture2D m_newVelocityBuffer;

    Texture2D m_divergenceBuffer;

    Texture2D m_curlBuffer;

    Texture2D m_pressureBuffer[2];

    DescriptorHeap m_renderDensityHeap;
    DescriptorHeap m_sourcingHeap;
    DescriptorHeap m_computeCurlHeap;
    DescriptorHeap m_vorticityConfinementHeap;

    DescriptorHeap m_advectionHeap;

    DescriptorHeap m_computeDivergenceHeap;
    DescriptorHeap m_jacobiHeap[2];

    DescriptorHeap m_computeFinalVelocityHeap;

    UINT gridWidth = 512;
    UINT gridHeight = 512;

    DXGI_FORMAT densityFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DXGI_FORMAT velocityFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DXGI_FORMAT divergenceFormat = DXGI_FORMAT_R32_FLOAT;
    DXGI_FORMAT pressureFormat = DXGI_FORMAT_R32_FLOAT;
    DXGI_FORMAT curlFormat = DXGI_FORMAT_R32_FLOAT;

    ConstantBuffer<SFLocalConstant> gridCB;
};
