#pragma once

#include "d3d12.h"
#include "directx/d3dx12.h"
#include "wrl.h"
#include <vector>

#include "GraphicsCommon.h"
#include "Mesh.h"
// #include "TextureGPUResource.h"
#include "Vertex.h"
#include "Core/VertexBuffer.h"
#include "Core/IndexBuffer.h"

class TextureLoader;

class StaticMesh
{

  public:
    StaticMesh();
    virtual ~StaticMesh();

    template <typename V, typename I>
    void Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList* commandList, Mesh<V, I>& mesh);

    template <typename V, typename I>
    void Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList* commandList,
                    const std::vector<Mesh<V, I>>& meshes);

    void InitializePoints(UINT64 pointCount);

    void Render(ID3D12GraphicsCommandList* commandList);
    void RenderPoints(ID3D12GraphicsCommandList* commandList);

    // vertex index 버퍼들의 upload 버퍼 clear
    void Clear();

    std::vector<D3D12_VERTEX_BUFFER_VIEW*>& GetVertexBufferView()
    {
        return m_vertexBufferViews;
    }
    std::vector<D3D12_INDEX_BUFFER_VIEW*>& GetIndexBufferView()
    {
        return m_indexBufferViews;
    }
    std::vector<UINT64>& GetIndexCounts()
    {
        return m_indexCounts;
    }

  protected:
    std::vector<VertexBuffer> m_vertexBuffers;
    std::vector<IndexBuffer> m_indexBuffers;

    std::vector<D3D12_VERTEX_BUFFER_VIEW*> m_vertexBufferViews;
    std::vector<D3D12_INDEX_BUFFER_VIEW*> m_indexBufferViews;
    std::vector<UINT64> m_indexCounts;

    UINT meshCount = 0;
};
#include "StaticMesh.inl"
