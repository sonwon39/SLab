#include "ModelLoader.h"
#include "GeometryGenerator.h"
#include "GraphicsCommon.h"
#include "Engine/World.h"

using namespace Graphics;

void ModelLoader<Vertex, uint16_t>::InitializeCPU()
{
    ID3D12Device5* device = m_world->GetDevice();
    Asset<Vertex, uint16_t> cube;
    cube.m_meshes.push_back({GeometryGenerator::MakeCube(1.f, 1.f, 1.f)});
    assets["cube"] = cube;

    float sphereRadius = 0.5f;
    int sphereDetail = 30;
    Asset<Vertex, uint16_t> sphere;
    sphere.m_meshes.push_back({GeometryGenerator::MakeSphere(sphereDetail, sphereRadius)});
    assets["sphere"] = sphere;

    int planeSize = 100;
    Asset<Vertex, uint16_t> plane;
    plane.m_meshes.push_back({GeometryGenerator::MakePlane((float)planeSize, (float)planeSize, 50)});
    assets["plane"] = plane;

    Asset<Vertex, uint16_t> rect;
    rect.m_meshes.push_back({GeometryGenerator::MakeRect(2.f, 2.f)});

    assets["rect"] = rect;
}

void ModelLoader<SimpleVertex, uint16_t>::InitializeCPU()
{
    ID3D12Device5* device = m_world->GetDevice();
    Asset<SimpleVertex, uint16_t> plane;
    plane.m_meshes.push_back({GeometryGenerator::MakeSimpleRect(2.f, 2.f)});

    Asset<SimpleVertex, uint16_t> cube;
    cube.m_meshes.push_back({GeometryGenerator::MakeSimpleCube(100.f, 100.f, 100.f)});

    assets["simple_plane"] = plane;
    assets["simple_cube"] = cube;
}

void ModelLoader<PBRVertex, uint16_t>::InitializeCPU()
{
    ID3D12Device5* device = m_world->GetDevice();
    Asset<PBRVertex, uint16_t> sphere;
    sphere.m_meshes.push_back({GeometryGenerator::PbrSphere(1.f, 100, 100)});

    Asset<PBRVertex, uint16_t> cube;
    cube.m_meshes.push_back({GeometryGenerator::PBRCube(2.f, 2.f, 2.f, 100, 100, 100)});

    int planeSize = 100;
    Asset<PBRVertex, uint16_t> plane;
    plane.m_meshes.push_back({GeometryGenerator::PBRPlane((float)planeSize, (float)planeSize,50, 50)});

	assets["pbr_plane"] = plane;
    assets["pbr_sphere"] = sphere;
    assets["pbr_cube"] = cube;
}
void ModelLoader<SkinnedVertex, uint16_t>::InitializeCPU()
{
}
    void ModelLoader<Vertex, uint16_t>::ProcessMesh(Asset<Vertex, uint16_t>& asset, aiMesh* mesh, const aiScene* scene,
                                                DirectX::SimpleMath::Matrix tr, bool loadAnimation)
{
    Mesh<Vertex, uint16_t> meshData;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        aiVector3D n = aiVector3D(0, 1, 0);
        if (mesh->HasNormals())
        {
            n = mesh->mNormals[i];
        }

        meshData.m_vertices.push_back({aiToVector3(v), aiToVector3(n), Vector2(0, 0)});
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            meshData.m_indices.push_back(face.mIndices[j]);
        }
    }

    asset.m_meshes.push_back(meshData);
}

void ModelLoader<PBRVertex, uint16_t>::ProcessMesh(Asset<PBRVertex, uint16_t>& asset, aiMesh* mesh,
                                                   const aiScene* scene, DirectX::SimpleMath::Matrix tr,
                                                   bool loadAnimation)
{
    Mesh<PBRVertex, uint16_t> meshData;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        aiVector3D n = aiVector3D(0, 1, 0);
        aiVector3D t = aiVector3D(1, 0, 0);
        aiVector3D uv = aiVector3D(0, 0, 0);
        if (mesh->HasNormals())
        {
            n = mesh->mNormals[i];
        }
        if (mesh->HasTangentsAndBitangents())
        {
            t = mesh->mTangents[i];
        }
        if (mesh->HasTextureCoords(0))
        {
            uv = mesh->mTextureCoords[0][i];
        }
        Vector3 vecUV = aiToVector3(uv);

        Vector3 pos = aiToVector3(v);
        Vector3 normal = aiToVector3(n);
        Vector3 tangent = aiToVector3(t);

        Matrix invTranspose = tr.Invert().Transpose();
        pos = Vector3::Transform(pos, tr);
        normal = Vector3::Transform(normal, invTranspose);
        tangent = Vector3::Transform(tangent, tr);

        normal.Normalize();
        tangent.Normalize();

        meshData.m_vertices.push_back({pos, normal, tangent, Vector2(vecUV.x, vecUV.y)});
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            meshData.m_indices.push_back(face.mIndices[j]);
        }
    }

    asset.m_meshes.push_back(meshData);
}

void ModelLoader<SkinnedVertex, uint16_t>::ProcessMesh(Asset<SkinnedVertex, uint16_t>& asset, aiMesh* mesh,
                                                       const aiScene* scene, DirectX::SimpleMath::Matrix tr,
                                                       bool loadAnimation)
{
    Mesh<SkinnedVertex, uint16_t> meshData;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        aiVector3D n = aiVector3D(0, 1, 0);
        aiVector3D t = aiVector3D(1, 0, 0);
        aiVector3D uv = aiVector3D(0, 0, 0);
        if (mesh->HasNormals())
        {
            n = mesh->mNormals[i];
        }
        if (mesh->HasTangentsAndBitangents())
        {
            t = mesh->mTangents[i];
        }
        if (mesh->HasTextureCoords(0))
        {
            uv = mesh->mTextureCoords[0][i];
        }
        Vector3 vecUV = aiToVector3(uv);

        Vector3 pos = aiToVector3(v);
        Vector3 normal = aiToVector3(n);
        Vector3 tangent = aiToVector3(t);

        Matrix invTranspose = tr.Invert().Transpose();
        pos = Vector3::Transform(pos, tr);
        normal = Vector3::Transform(normal, invTranspose);
        tangent = Vector3::Transform(tangent, tr);

        normal.Normalize();
        tangent.Normalize();

        SkinnedVertex vertex;
        vertex.position = pos;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.texcoord = Vector2(vecUV.x, vecUV.y);

        meshData.m_vertices.push_back({vertex});
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            meshData.m_indices.push_back(face.mIndices[j]);
        }
    }
    std::vector<std::vector<float>> weights(meshData.m_vertices.size());
    std::vector<std::vector<uint32_t>> indices(meshData.m_vertices.size());
    if (loadAnimation && mesh->HasBones())
    {
        asset.animData.boneOffset.resize(asset.animData.boneNameToId.size());
        asset.animData.boneTransform.resize(asset.animData.boneNameToId.size());

        for (size_t i = 0; i < mesh->mNumBones; i++)
        {
            const aiBone* bone = mesh->mBones[i];
            std::string boneName = bone->mName.C_Str();
            uint32_t boneId = asset.animData.boneNameToId[boneName];
            asset.animData.boneOffset[boneId] = Matrix((float*)&bone->mOffsetMatrix).Transpose();
            for (size_t j = 0; j < bone->mNumWeights; j++)
            {
                auto weight = bone->mWeights[j];
                weights[weight.mVertexId].push_back(weight.mWeight);
                indices[weight.mVertexId].push_back(boneId);
            }
        }
        for (size_t i = 0; i < meshData.m_vertices.size(); i++)
        {
            for (size_t j = 0; j < weights[i].size(); j++)
            {
                if (j >= 8)
                {
                    continue;
                }
                meshData.m_vertices[i].blendWeights[j] = weights[i][j];
                meshData.m_vertices[i].boneIndices[j] = indices[i][j];
            }
        }
    }

    asset.m_meshes.push_back(meshData);
}

void ModelLoader<SkinnedVertex, uint32_t>::ProcessMesh(Asset<SkinnedVertex, uint32_t>& asset, aiMesh* mesh,
                                                       const aiScene* scene, DirectX::SimpleMath::Matrix tr,
                                                       bool loadAnimation)
{
    Mesh<SkinnedVertex, uint32_t> meshData;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        aiVector3D n = aiVector3D(0, 1, 0);
        aiVector3D t = aiVector3D(1, 0, 0);
        aiVector3D uv = aiVector3D(0, 0, 0);
        if (mesh->HasNormals())
        {
            n = mesh->mNormals[i];
        }
        if (mesh->HasTangentsAndBitangents())
        {
            t = mesh->mTangents[i];
        }
        if (mesh->HasTextureCoords(0))
        {
            uv = mesh->mTextureCoords[0][i];
        }
        Vector3 vecUV = aiToVector3(uv);

        Vector3 pos = aiToVector3(v);
        Vector3 normal = aiToVector3(n);
        Vector3 tangent = aiToVector3(t);

        Matrix invTranspose = tr.Invert().Transpose();
        pos = Vector3::Transform(pos, tr);
        normal = Vector3::Transform(normal, invTranspose);
        tangent = Vector3::Transform(tangent, tr);

        normal.Normalize();
        tangent.Normalize();

        SkinnedVertex vertex;
        vertex.position = pos;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.texcoord = Vector2(vecUV.x, vecUV.y);

        meshData.m_vertices.push_back({vertex});
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            meshData.m_indices.push_back(face.mIndices[j]);
        }
    }
    std::vector<std::vector<float>> weights(meshData.m_vertices.size());
    std::vector<std::vector<uint32_t>> indices(meshData.m_vertices.size());
    if (loadAnimation && mesh->HasBones())
    {
        asset.animData.boneOffset.resize(asset.animData.boneNameToId.size());
        asset.animData.boneTransform.resize(asset.animData.boneNameToId.size());

        for (size_t i = 0; i < mesh->mNumBones; i++)
        {
            const aiBone* bone = mesh->mBones[i];
            std::string boneName = bone->mName.C_Str();
            uint32_t boneId = asset.animData.boneNameToId[boneName];
            asset.animData.boneOffset[boneId] = Matrix((float*)&bone->mOffsetMatrix).Transpose();
            for (size_t j = 0; j < bone->mNumWeights; j++)
            {
                auto weight = bone->mWeights[j];
                weights[weight.mVertexId].push_back(weight.mWeight);
                indices[weight.mVertexId].push_back(boneId);
            }
        }
        for (size_t i = 0; i < meshData.m_vertices.size(); i++)
        {
            for (size_t j = 0; j < weights[i].size(); j++)
            {
                if (j >= 8)
                {
                    continue;
                }
                meshData.m_vertices[i].blendWeights[j] = weights[i][j];
                meshData.m_vertices[i].boneIndices[j] = indices[i][j];
            }
        }
    }

    asset.m_meshes.push_back(meshData);
}
