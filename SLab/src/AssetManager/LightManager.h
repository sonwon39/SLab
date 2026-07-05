#pragma once

#include "directxtk12\SimpleMath.h"
#include "GlobalConstant.h"
#include <array>
#include <memory>
#include "Core/StructuredBuffer.h"

// 라이트들에 대한 모든 데이터를 관리하는 Light Manager
// LightComponent가 등록을 요청하면 slot만 넘겨 주고
// 실제 관리는 데이터관리는 여기서 한다.
class LightManager
{
  public:
    LightManager();
    virtual ~LightManager();

  public:
    void Initialize(D3D12_RESOURCE_FLAGS flags, ID3D12GraphicsCommandList* commandList, const std::wstring& name);
    int Allocate();
    void Update();
    D3D12_GPU_VIRTUAL_ADDRESS GetLightView() const
    {
        return m_lightBuffer->GetGPUVirtualAddress();
    }
    void Bind(const RootSignature* rs, ID3D12GraphicsCommandList* cl) const;
   
    const std::array<Light, MAX_LIGHT>& GetLights()
    {
        return m_lights;
    }
    Light& At(int slot) 
    {
        return m_lights[slot];
    }

  private:
    std::array<Light, MAX_LIGHT> m_lights;
    std::shared_ptr<StructuredBuffer> m_lightBuffer;
    uint64_t dataSize = 0;

  private:
    uint16_t m_lightNum = 0;
};
