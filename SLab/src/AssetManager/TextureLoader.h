#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include "wrl.h"
#include "d3d12.h"
#include "DescriptorHeap.h"
#include "Core/Texture2D.h"

struct TextureInfo
{
    uint64_t offset;
    uint64_t size;
};

class TextureLoader
{
  public:
    TextureLoader();
    TextureLoader(std::string path);
    virtual ~TextureLoader();

    void InitHeap(UINT heapSize);
    void LoadIdx();

    void LoadTextures(ID3D12GraphicsCommandList* commandList);
    void AddTexture(const std::string& textureName, Texture2D& texture);
    void ClearBlobs();

  public:
    DescriptorHeap* GetDescriptorHeap()
    {
        return &heap;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const int& idx) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const std::string& filename) const;
    ID3D12Resource* GetTexture(const std::string& filename) const;
    int GetTextureIndex(const std::string& filename) const;
    std::vector<std::string> filenames;

  private:
    std::string folder;
    std::string binPath;
    std::string idxPath;

    uint32_t count;

    std::unordered_map<std::string, TextureInfo> textureMap;
    std::unordered_map<uint32_t, std::string> nameMap;
    std::map<std::string, uint32_t> idxMap;

    std::vector<Texture2D> textures;
    DescriptorHeap heap;

    UINT srvOffset = 0;
};
