#pragma once

#include "d3d12.h"
#include "directxtk12\SimpleMath.h"
#include <vector>

#include "Core/Texture2D.h"
#include "Core/StructuredBuffer.h"
#include "Core/ConstantBuffer.h"
#include "DescriptorHeap.h"
#include "NoiseLocalConstant.h"

class Noise
{
  public:
    Noise();
    virtual ~Noise();

    void Initialize(UINT width, UINT height, ID3D12GraphicsCommandList* cmdList);
    void InitGPU(UINT width, UINT height, ID3D12GraphicsCommandList* cmdList);
    void GeneratePerlinNoise();
    void GenerateCurlNoise();


	// partice 위치 갱신
    void CurlNoiseSimulation(float deltaTime);
    void Sourcing();
	// curl noise 를 사용한 particle 이동
    void ParticleAdvection();
    void RenderParticles(ID3D12GraphicsCommandList* c);

    void ResetCommand();
    void InitCommands();
    void InitCPU();

  public:
    void Execute(ID3D12CommandQueue* commandQueue);
    void Dispatch(UINT width, UINT height, UINT gs_x, UINT gs_y);
    void Dispatch(UINT count, UINT gs_x);
    void SetCPSO(const std::string psoName);

  private:
    DescriptorHeap m_noiseHeap;
    Texture2D m_perlinNoise;
    Texture2D m_curlNoise;
    Texture2D m_density;

    UINT perlinWidth = 256;
    UINT perlinHeight = 256;

    // curlNoise (속도장) 크기
    UINT m_width = 0;
    UINT m_height = 0;

  private:
    UINT particleCount = 150;
    StructuredBuffer particles;
    std::vector<NoiseParticle> particleCPU;
    ConstantBuffer<NoiseLocalConstant> m_noiseLCB;

  private:
    std::string perlinCPSOName = "perlinNoiseCPSO";
    std::string curlCPSOName = "curlNoiseCPSO";
    std::string curlSimulationCPSOName = "curlSimulationCPSO";
    std::string noiseSourcingCPSOName = "noiseSourcingCPSO";

  private:
    DXGI_FORMAT perlinFormat = DXGI_FORMAT_R16_FLOAT;
    DXGI_FORMAT curlFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT densityFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

  private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

	
};
