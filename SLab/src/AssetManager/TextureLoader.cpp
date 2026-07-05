#include "TextureLoader.h"
#include <omp.h>

#include "Renderer.h"
#include "GraphicsCommon.h"
#include "Engine/World.h"
#include "DataType.h"

using namespace Graphics;

namespace fs = std::filesystem;

std::ostream& operator<<(std::ostream& out, const TextureInfo& info)
{
    out << ", Offset : " << info.offset << " , size : " << info.size;
    out << '\n';

    return out;
}

TextureLoader::TextureLoader()
{
}

TextureLoader::TextureLoader(std::string path) : folder(path)
{
    count = 0;
    binPath = folder + "textures.bin";
    idxPath = folder + "textures.idx";
}

TextureLoader::~TextureLoader()
{
    textures.clear();
}

void TextureLoader::InitHeap(UINT heapSize)
{
    heap.Initialize(heapSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}

void TextureLoader::LoadIdx()
{
    std::ifstream idx(idxPath.c_str(), std::ios::binary);

    idx.read(reinterpret_cast<char*>(&count), sizeof(count));

    std::cout << "dds count : " << count << '\n';

    for (uint32_t i = 0; i < count; i++)
    {
        TextureInfo info;
        uint32_t nameLen;

        idx.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        std::string filename(nameLen, ' ');

        idx.read(filename.data(), nameLen);
        idx.read(reinterpret_cast<char*>(&info.offset), sizeof(info.offset));
        idx.read(reinterpret_cast<char*>(&info.size), sizeof(info.size));

        // std::cout << "File name : " << filename << info;
        textureMap[filename] = info;
        nameMap[i] = filename;
        idxMap[filename] = i;
        filenames.push_back(filename);
    }
}

void TextureLoader::LoadTextures(ID3D12GraphicsCommandList* commandList)
{
    ID3D12Device5* m_device = m_world->GetDevice();
    srvOffset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    textures.resize(count);

    std::ifstream bin(binPath, std::ios::binary);

    for (uint32_t i = 0; i < count; i++)
    {
        std::string filename = nameMap[i];

        bool isCubeMap = filename.find("CubeMap") != std::string::npos;
        bool isAlbedo =
            (filename.find("albedo") != std::string::npos) || (filename.find("Albedo") != std::string::npos);

        TextureInfo info = textureMap.at(filename);
        uint64_t size = info.size;

        bin.seekg(info.offset);

        DDS_LOADER_FLAGS flags = isAlbedo ? DirectX::DX12::DDS_LOADER_FORCE_SRGB : DirectX::DX12::DDS_LOADER_DEFAULT;

        textures[i].Initialize(bin, size, flags, commandList);

        if (isCubeMap)
            heap.CreateResourceView(textures[i].Get(), DescriptorType::SRV, ViewDimensionType::TEXTURECUBE);
        else
            heap.CreateResourceView(textures[i].Get(), DescriptorType::SRV, ViewDimensionType::TEXTURE2D);
    }
}

void TextureLoader::AddTexture(const std::string& textureName, Texture2D& texture)
{
    uint32_t i = count;
    nameMap[i] = textureName;
    idxMap[textureName] = i;
    filenames.push_back(textureName);
    count++;

    heap.CreateResourceView(texture.Get(), DescriptorType::SRV, ViewDimensionType::TEXTURE2D);
    textures.push_back(texture);
}

void TextureLoader::ClearBlobs()
{
    for (uint32_t i = 0; i < count; i++)
    {
        textures[i].Clear();
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureLoader::GetGPUHandle(const int& idx) const
{
    return heap.GetGPUHandle(idx);
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureLoader::GetGPUHandle(const std::string& filename) const
{
    uint32_t idx = 0;
    auto it = idxMap.find(filename);
    if (it != idxMap.end())
    {
        idx = it->second;
    }
    return heap.GetGPUHandle(idx);
}

ID3D12Resource* TextureLoader::GetTexture(const std::string& filename) const
{
    const std::string test;
    uint32_t idx = 0;
    auto it = idxMap.find(filename);
    if (it != idxMap.end())
    {
        idx = it->second;
    }
    return textures[idx].Get();
}

int TextureLoader::GetTextureIndex(const std::string& filename) const
{
    const std::string test;
    int idx = -1;
    auto it = idxMap.find(filename);
    if (it != idxMap.end())
    {
        idx = it->second;
    }
    return idx;
}
