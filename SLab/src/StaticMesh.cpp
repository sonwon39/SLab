#include "StaticMesh.h"
#include "Vertex.h"

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::Clear()
{
    for (auto& v : m_vertexBuffers)
    {
        v.Clear();
        m_vertexBufferViews.push_back(v.GetView());
    }

    for (auto& i : m_indexBuffers)
    {
        m_indexBufferViews.push_back(i.GetView());
        m_indexCounts.push_back(i.dataCount);
        i.Clear();
    }
}

void StaticMesh::InitializePoints(UINT64 pointCount)
{
    m_indexCounts.push_back(pointCount);
}

void StaticMesh::Render(ID3D12GraphicsCommandList* commandList)
{
    for (size_t i = 0; i < meshCount; i++)
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, m_vertexBuffers[i].GetView());
        commandList->IASetIndexBuffer(m_indexBuffers[i].GetView());
        UINT indexCount = (UINT)m_indexBuffers[i].dataCount;
        commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
}

void StaticMesh::RenderPoints(ID3D12GraphicsCommandList* c)
{
    c->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    c->DrawInstanced((UINT)m_indexCounts[0], 1, 0, 0);
}
