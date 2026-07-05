#include "GeometryGenerator.h"
#include <iostream>

Mesh<SimpleVertex, uint16_t> GeometryGenerator::MakeSimpleRect(float x, float y)
{
    float halfX = x / 2.f;
    float halfY = y / 2.f;
    std::vector<SimpleVertex> vertices{{Vector3(-halfX, -halfY, 1), Vector2(0, 1)},
                                       {Vector3(-halfX, halfY, 1), Vector2(0, 0)},
                                       {Vector3(halfX, halfY, 1), Vector2(1, 0)},
                                       {Vector3(halfX, -halfY, 1), Vector2(1, 1)}};
    std::vector<uint16_t> indices{0, 1, 2, 0, 2, 3};

    Mesh<SimpleVertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<SimpleVertex, uint16_t> GeometryGenerator::MakeSimpleCube(float x, float y, float z)
{
    float halfX = x / 2.f;
    float halfY = y / 2.f;
    float halfZ = z / 2.f;
    std::vector<Vector3> v{Vector3(-halfX, -halfY, -halfZ), Vector3(-halfX, halfY, -halfZ),
                           Vector3(halfX, halfY, -halfZ),   Vector3(halfX, -halfY, -halfZ),
                           Vector3(-halfX, -halfY, halfZ),  Vector3(-halfX, halfY, halfZ),
                           Vector3(halfX, halfY, halfZ),    Vector3(halfX, -halfY, halfZ)};
    std::vector<std::tuple<int, int, int, int>> vSet{
        {0, 1, 2, 3}, // front
        {3, 2, 6, 7}, // right
        {4, 5, 1, 0}, // left
        {7, 6, 5, 4}, // back
        {4, 0, 3, 7}, // bottom
        {1, 5, 6, 2}, // top
    };

    std::vector<SimpleVertex> vertices;
    std::vector<uint16_t> indices;

    for (size_t i = 0; i < vSet.size(); i++)
    {
        auto& [v0, v1, v2, v3] = vSet[i];

        vertices.push_back({v[v0], Vector2(0, 1)});
        vertices.push_back({v[v1], Vector2(0, 0)});
        vertices.push_back({v[v2], Vector2(1, 0)});
        vertices.push_back({v[v3], Vector2(1, 1)});

        size_t base = i * 4;
        indices.push_back(uint16_t(base + 0));
        indices.push_back(uint16_t(base + 1));
        indices.push_back(uint16_t(base + 2));

        indices.push_back(uint16_t(base + 0));
        indices.push_back(uint16_t(base + 2));
        indices.push_back(uint16_t(base + 3));
    }

    Mesh<SimpleVertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<Vertex, uint16_t> GeometryGenerator::MakeCube(float x, float y, float z)
{

    float halfX = x / 2.f;
    float halfY = y / 2.f;
    float halfZ = z / 2.f;
    std::vector<Vector3> v{Vector3(-halfX, -halfY, -halfZ), Vector3(-halfX, halfY, -halfZ),
                           Vector3(halfX, halfY, -halfZ),   Vector3(halfX, -halfY, -halfZ),
                           Vector3(-halfX, -halfY, halfZ),  Vector3(-halfX, halfY, halfZ),
                           Vector3(halfX, halfY, halfZ),    Vector3(halfX, -halfY, halfZ)};
    std::vector<std::tuple<int, int, int, int>> vSet{
        {0, 1, 2, 3}, // front
        {3, 2, 6, 7}, // right
        {4, 5, 1, 0}, // left
        {7, 6, 5, 4}, // back
        {4, 0, 3, 7}, // bottom
        {1, 5, 6, 2}, // top
    };
    std::vector<Vector3> nSet{
        {0, 0, -1}, // front
        {1, 0, 0},  // right
        {-1, 0, 0}, // left
        {0, 0, 1},  // back
        {0, -1, 0}, // bottom
        {0, 1, 0},  // top
    };
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    for (uint16_t i = 0; i < vSet.size(); i++)
    {
        auto& [v0, v1, v2, v3] = vSet[i];

        vertices.push_back({v[v0], nSet[i], Vector2(0, 1)});
        vertices.push_back({v[v1], nSet[i], Vector2(0, 0)});
        vertices.push_back({v[v2], nSet[i], Vector2(1, 0)});
        vertices.push_back({v[v3], nSet[i], Vector2(1, 1)});

        uint16_t base = i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    Mesh<Vertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<Vertex, uint16_t> GeometryGenerator::MakePlane(float x, float z, int c)
{
    float halfX = x / 2.f;
    float halfZ = z / 2.f;

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    Vector3 baseV = Vector3(-halfX, 0, halfZ);
    float delX = x / c;
    float delZ = -z / c;

    float deluv = 1.f / c;
    for (int i = 0; i <= c; i++)
    {

        Vector3 xBase = baseV + Vector3(0.f, 0.f, delZ) * (float)i;
        for (int j = 0; j <= c; j++)
        {
            Vector3 v = xBase + (float)j * Vector3(delX, 0, 0);
            vertices.push_back({v, Vector3(0, 1, 0), Vector2(deluv * j, deluv * i)});
            if (j != c && i != c)
            {
                uint16_t a = (c + 1) * i + j;
                uint16_t b = a + c + 1;
                uint16_t c = a + 1;
                uint16_t d = b + 1;
                indices.push_back(b);
                indices.push_back(a);
                indices.push_back(c);

                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    Mesh<Vertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<Vertex, uint16_t> GeometryGenerator::MakeRect(float x, float y)
{
    float halfX = x / 2.f;
    float halfY = y / 2.f;
    Vector3 n = Vector3(0.f, 0.f, -1.f);
    std::vector<Vertex> vertices{{Vector3(-halfX, -halfY, 1), n, Vector2(0, 1)},
                                 {Vector3(-halfX, halfY, 1), n, Vector2(0, 0)},
                                 {Vector3(halfX, halfY, 1), n, Vector2(1, 0)},
                                 {Vector3(halfX, -halfY, 1), n, Vector2(1, 1)}};
    std::vector<uint16_t> indices{0, 1, 2, 0, 2, 3};

    Mesh<Vertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<Vertex, uint16_t> GeometryGenerator::MakeSphere(int c, float r)
{
    float pi = (float)std::acos(-1);
    float thetaZ = pi / c;
    float thetaY = pi * 2.f / c;

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    Vector3 baseZ = Vector3(0, r, 0);

    float deluv = 1.f / c;
    for (int i = 0; i <= c; i++)
    {

        DirectX::SimpleMath::Matrix zMat = DirectX::XMMatrixRotationZ(thetaZ * i);
        Vector3 baseY = Vector3::Transform(baseZ, zMat);
        for (int j = 0; j <= c; j++)
        {
            DirectX::SimpleMath::Matrix yMat = DirectX::XMMatrixRotationY(-thetaY * j);
            Vector3 v = Vector3::Transform(baseY, yMat);
            Vector3 n = v;
            n.Normalize();
            vertices.push_back({v, n, Vector2(deluv * j, deluv * i)});
            if (j != c && i != c)
            {
                uint16_t i0 = (c + 1) * i + j;
                uint16_t i1 = i0 + c + 1;
                uint16_t i2 = i0 + 1;
                uint16_t i3 = i1 + 1;
                indices.push_back(i1);
                indices.push_back(i0);
                indices.push_back(i2);

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
    }

    Mesh<Vertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<PointCloudVertex, uint16_t> GeometryGenerator::MakePointCube(float x, float y, float z)
{
    float halfX = x / 2.f;
    float halfY = y / 2.f;
    float halfZ = z / 2.f;
    std::vector<Vector3> v{Vector3(-halfX, -halfY, -halfZ), Vector3(-halfX, halfY, -halfZ),
                           Vector3(halfX, halfY, -halfZ),   Vector3(halfX, -halfY, -halfZ),
                           Vector3(-halfX, -halfY, halfZ),  Vector3(-halfX, halfY, halfZ),
                           Vector3(halfX, halfY, halfZ),    Vector3(halfX, -halfY, halfZ)};

    std::vector<PointCloudVertex> vertices;
    std::vector<uint16_t> indices;

    for (size_t i = 0; i < v.size(); i++)
    {
        vertices.push_back({v[i], Vector4(1, 1, 1, 1)});
    }
    Mesh<PointCloudVertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<SimpleVertex, uint16_t> GeometryGenerator::MakePoint()
{

    Mesh<SimpleVertex, uint16_t> mesh;
    std::vector<SimpleVertex> vertices;
    std::vector<uint16_t> indices;

    vertices.push_back({Vector3(0, 0, 0)});
    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<PositionVertex, uint16_t> GeometryGenerator::MakePositionPoint()
{

    Mesh<PositionVertex, uint16_t> mesh;
    std::vector<PositionVertex> vertices;
    std::vector<uint16_t> indices;

    vertices.push_back({Vector3(0, 0, 0)});
    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<PBRVertex, uint16_t> GeometryGenerator::PBRPlane(float halfX, float halfZ, int cX, int cZ)
{
    std::vector<PBRVertex> vertices;
    std::vector<uint16_t> indices;

    Vector3 baseV = Vector3(-halfX, 0, halfZ);
    float delX = halfX * 2.f / cX;
    float delZ = -halfZ * 2.f / cZ;

    float deluvX = 1.f / cX;
    float deluvZ = 1.f / cZ;
    for (int i = 0; i <= cZ; i++)
    {

        Vector3 xBase = baseV + Vector3(0.f, 0.f, delZ) * (float)i;
        for (int j = 0; j <= cX; j++)
        {
            Vector3 v = xBase + (float)j * Vector3(delX, 0, 0);
            PBRVertex vertex;
            vertex.position = v;
            vertex.normal = Vector3(0.f, 1.f, 0.f);
            vertex.uv = Vector2(deluvX * j, deluvZ * i);
            vertex.tangent = Vector3(1, 0, 0);

            vertices.push_back(vertex);

            if (j != cX && i != cZ)
            {
                uint16_t a = (cX + 1) * i + j;
                uint16_t b = a + cX + 1;
                uint16_t c = a + 1;
                uint16_t d = b + 1;
                indices.push_back(b);
                indices.push_back(a);
                indices.push_back(c);

                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    Mesh<PBRVertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}
Mesh<PBRVertex, uint16_t> GeometryGenerator::PbrSphere(const float& radius, const int& x, const int& y)
{
    Mesh<PBRVertex, uint16_t> mesh;

    Vector3 basePosition(Vector3(0.f, radius, 0.f));

    float delYTheta = DirectX::XM_2PI / x;
    float delZTheta = DirectX::XM_PI / y;

    float uvDelX = 1.f / x;
    float uvDelY = 1.f / y;

    int index = 0;

    std::vector<PBRVertex> vertices;
    std::vector<uint16_t> indices;
    for (int j = 0; j < y + 1; ++j)
    {
        Vector3 position = Vector3::Transform(basePosition, DirectX::XMMatrixRotationZ(-delZTheta * j));
        for (int i = 0; i < x + 1; ++i)
        {
            if (j == 0 || j == y)
            {
                if (i == x)
                {
                    break;
                }
            }

            PBRVertex v;
            v.position = Vector3::Transform(position, DirectX::XMMatrixRotationY(-delYTheta * i));
            v.uv = Vector2(i * uvDelX, j * uvDelY);
            v.normal = v.position;
            v.normal.Normalize();

            vertices.push_back(v);
        }
    }

    for (int i = 0; i < y; i++)
    {
        int index = i * (x + 1) - 1;
        for (int j = 0; j < x; j++)
        {
            int idx = index + j;
            if (i == 0)
            {
                indices.push_back(idx + x + 1);
                indices.push_back(idx + 1);
                indices.push_back(idx + x + 2);
            }
            else if (i == y - 1)
            {
                indices.push_back(index + x + 1);
                indices.push_back(idx);
                indices.push_back(idx + 1);
            }
            else
            {
                indices.push_back(idx + x + 1);
                indices.push_back(idx);
                indices.push_back(idx + 1);

                indices.push_back(idx + x + 1);
                indices.push_back(idx + 1);
                indices.push_back(idx + x + 2);
            }
        }
    }
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        int idx0 = indices[i];
        int idx1 = indices[i + 1];
        int idx2 = indices[i + 2];

        PBRVertex& v0 = vertices[idx0];
        PBRVertex& v1 = vertices[idx1];
        PBRVertex& v2 = vertices[idx2];

        ComputeTangent(v0, v1, v2);
    }
    // vertices[0].tangent = Vector3(0, 0, 0);
    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

Mesh<PBRVertex, uint16_t> GeometryGenerator::PBRCube(float x, float y, float z, int xCount, int yCount, int zCount)
{
    std::vector<PBRVertex> vertices;
    std::vector<uint16_t> indices;
    float halfX = x / 2.f;
    float halfY = y / 2.f;
    float halfZ = z / 2.f;

    std::vector<std::tuple<float, float, int, int>> halfList = {{halfX, halfZ, xCount, zCount},
                                                                {halfX, halfY, xCount, yCount},
                                                                {halfX, halfY, xCount, yCount},
                                                                {halfZ, halfY, zCount, yCount},
                                                                {halfZ, halfY, zCount, yCount},
                                                                {halfX, halfZ, xCount, zCount}

    };
    DirectX::SimpleMath::Matrix frontMat = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
    // 위 앞 뒤 좌 우 아래
    std::vector<DirectX::SimpleMath::Matrix> matList = {
        DirectX::XMMatrixTranslation(0, halfY, 0),                                                              // 위
        frontMat * DirectX::XMMatrixTranslation(0, 0, -halfZ),                                                  // 앞
        frontMat * DirectX::XMMatrixRotationY(DirectX::XM_PI) * DirectX::XMMatrixTranslation(0, 0, halfZ),      // 뒤
        frontMat * DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2) * DirectX::XMMatrixTranslation(-halfX, 0, 0), // 좌
        frontMat * DirectX::XMMatrixRotationY(-DirectX::XM_PIDIV2) * DirectX::XMMatrixTranslation(halfX, 0, 0), // 우
        DirectX::XMMatrixRotationX(-DirectX::XM_PI) * DirectX::XMMatrixTranslation(0, -halfY, 0),               // 아래
    };

    uint16_t offset = 0;
    for (size_t i = 0; i < 6; i++)
    {
        Mesh<PBRVertex, uint16_t> m = PBRPlane(std::get<0>(halfList[i]), std::get<1>(halfList[i]),
                                               std::get<2>(halfList[i]), std::get<3>(halfList[i]));

        for (auto& v : m.m_vertices)
        {
            v.position = Vector3::Transform(v.position, matList[i]);
            v.normal = Vector3::Transform(v.normal, matList[i]);
            v.tangent = Vector3::Transform(v.tangent, matList[i]);

            v.normal.Normalize();
            v.tangent.Normalize();
            vertices.push_back(v);
        }
        for (auto& i : m.m_indices)
        {
            i += offset;
            indices.push_back(i);
        }
        offset += (uint16_t)m.m_vertices.size();
    }

    Mesh<PBRVertex, uint16_t> mesh;

    mesh.m_vertices = vertices;
    mesh.m_indices = indices;

    return mesh;
}

void GeometryGenerator::ComputeTangent(PBRVertex& v0, PBRVertex& v1, PBRVertex& v2)
{
    DirectX::SimpleMath::Vector2 t0 = v1.uv - v0.uv;
    DirectX::SimpleMath::Vector2 t1 = v2.uv - v0.uv;

    Vector3 e0 = v1.position - v0.position;
    Vector3 e1 = (v2.position - v0.position);

    float a = t0.x;
    float b = t0.y;
    float c = t1.x;
    float d = t1.y;

    float det = a * d - b * c;
    float invDet = 1.f / det;

    Vector3 tangent = invDet * (e0 * d - b * e1);
    tangent.Normalize();

    v0.tangent = tangent;
    v1.tangent = tangent;
    v2.tangent = tangent;
}
