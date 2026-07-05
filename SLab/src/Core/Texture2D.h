#pragma once

#include "wrl.h"
#include "d3d12.h"
#include <string>
#include <vector>
#include "Directxtk12/DDSTextureLoader.h"
#include <fstream>
#include "GPUBuffer.h"

class Texture2D : public GPUBuffer
{

  public:
    Texture2D();
    virtual ~Texture2D();

  public:
    void Initialize(int width, int height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state,
                    UINT miplevels, std::wstring name);
    void Initialize(std::ifstream& bin, uint64_t size, DirectX::DX12::DDS_LOADER_FLAGS flags,
                    ID3D12GraphicsCommandList* commandList, std::wstring name = L"");
    virtual void Clear();

  public:
    int m_width;
    int m_height;

  private:
    std::vector<uint8_t> ddsBlob;
};
