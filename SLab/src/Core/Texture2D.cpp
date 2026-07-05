#include "Texture2D.h"
#include "GraphicsCommon.h"

using namespace Graphics;

Texture2D::Texture2D()
{
}

Texture2D::~Texture2D()
{
    Clear();
}

void Texture2D::Clear()
{
    GPUBuffer::Clear();
    ddsBlob.clear();
}

void Texture2D::Initialize(int width, int height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
                           D3D12_RESOURCE_STATES state, UINT miplevels, std::wstring name)
{
    m_width = width;
    m_height = height;

    utility->CreateTextureBuffer(gpu, width, height, format, flags, state, miplevels, name);

    m_currentState = state;
}

void Texture2D::Initialize(std::ifstream& bin, uint64_t size, DirectX::DX12::DDS_LOADER_FLAGS flags,
                           ID3D12GraphicsCommandList* commandList, std::wstring name)
{
    bufferSize = size;
    ddsBlob.reserve(size);
    bin.read(reinterpret_cast<char*>(ddsBlob.data()), size);
    utility->CreateTextureFromDDS(ddsBlob.data(), size, gpu, upload, flags, commandList);
}
