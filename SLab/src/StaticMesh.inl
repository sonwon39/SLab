#pragma once

#include "StaticMesh.h"
#include "GraphicsCommon.h"

template <typename V, typename I>
inline void StaticMesh::Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList* commandList, Mesh<V, I>& mesh)
{
    meshCount = 1;
    VertexBuffer v;
    IndexBuffer i;
    v.Initialize<V>(mesh.m_vertices, D3D12_RESOURCE_FLAG_NONE, commandList);
    i.Initialize<I>(mesh.m_indices, D3D12_RESOURCE_FLAG_NONE, commandList);

    m_vertexBuffers.push_back(v);
    m_indexBuffers.push_back(i);
}

template <typename V, typename I>
inline void StaticMesh::Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList* commandList,
                                   const std::vector<Mesh<V, I>>& meshes)
{
    meshCount = (UINT)meshes.size();

    for (size_t i = 0; i < meshCount; i++)
    {
        const auto& mesh = meshes[i];
        VertexBuffer vertexBuffer;
        IndexBuffer indexBuffer;
        vertexBuffer.Initialize<V>(mesh.m_vertices, D3D12_RESOURCE_FLAG_NONE, commandList);
        indexBuffer.Initialize<I>(mesh.m_indices, D3D12_RESOURCE_FLAG_NONE, commandList);

        m_vertexBuffers.push_back(vertexBuffer);
        m_indexBuffers.push_back(indexBuffer);
    }
}
