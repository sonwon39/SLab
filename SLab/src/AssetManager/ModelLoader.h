#pragma once

#include "d3d12.h"
#include "wrl.h"

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <omp.h>

#include "Vertex.h"
#include "Asset.h"
#include "StaticMesh.h"
#include "BlendData.h"

#include "Core/ConstantBuffer.h"

template <typename V, typename I> class ModelLoader
{
  public:
    ModelLoader()
    {
        basePath = std::filesystem::current_path().string();
        basePath += "/Assets/Models/";
    };
    ~ModelLoader() {};

  public:
    void InitializeCPU();
    void InitializeGPU(ID3D12GraphicsCommandList* commandList);

  public:
    std::string basePath;
    std::vector<Mesh<V, I>> GetAsset(const std::string& assetName) const;
    //std::shared_ptr<StaticMesh> GetMeshes(const std::string& assetName) const;
    DirectX::SimpleMath::Vector3 aiToVector3(aiVector3D vector);
    SkinnedLocalConstant GetCurrentSLC(const float& frame, const std::string& assetName, const int& clipIdx,
                                       bool updateRootPos);
    void UpdateSLC(const float& frame, const std::string& assetName, const int& clipIdx, bool updateRootPos,
                   ConstantBuffer<SkinnedLocalConstant>& cb);
    SkinnedLocalConstant BlendPose(const BlendData& blendData);
    size_t GetAnimationSize(const std::string& assetName, const int& clipIdx) const;

    DirectX::SimpleMath::Matrix GetBoneTransform(const std::string& assetName, const std::string& boneName,
                                                 const Matrix& socketTransform, const int& clipIdx);

    DirectX::SimpleMath::Quaternion aiToQuaternion(aiQuaternion quat);
    DirectX::SimpleMath::Vector4 aiToVector4(aiColor4D vector);
    aiMatrix4x4 MatrixToAi(const DirectX::SimpleMath::Matrix& mat);
    DirectX::SimpleMath::Matrix aiToMatrix(const aiMatrix4x4& mat);

  private:
    std::unordered_map<std::string, Asset<V, I>> assets;
    // std::unordered_map<std::string, std::shared_ptr<StaticMesh>> meshesMap;

  public:
    void Load(std::string filename, DirectX::SimpleMath::Matrix tr = DirectX::SimpleMath::Matrix(),
              bool loadAnimation = false);
    void LoadAnimations(std::string folderPath, std::string targetAssetName);
    void LoadAnimation(std::string filePath, std::string targetAssetName, bool useBasePath = true);

    const aiNode* FindParent(Asset<V, I>& asset, const aiNode* node);
    void ProcessNode(Asset<V, I>& asset, aiNode* node, const aiScene* scene, DirectX::SimpleMath::Matrix tr,
                     bool loadAnimation);
    void ProcessMesh(Asset<V, I>& asset, aiMesh* mesh, const aiScene* scene, DirectX::SimpleMath::Matrix tr,
                     bool loadAnimation);

    void LoadPointCloud(std::string filename, DirectX::SimpleMath::Matrix tr = DirectX::SimpleMath::Matrix());
    void ProcessPCNode(std::vector<Mesh<V, I>>& meshes, aiNode* node, const aiScene* scene,
                       DirectX::SimpleMath::Matrix tr);
    void ProcessPCMesh(std::vector<Mesh<V, I>>& meshes, aiMesh* mesh, const aiScene* scene,
                       DirectX::SimpleMath::Matrix tr);

  private:
    void CreateBoneTree(Asset<V, I>& asset, aiNode* node, uint32_t* count);
    void GetBonePosition(Asset<V, I>& asset, aiNode* node, const aiMatrix4x4& parentTransform, const aiScene* scene);
};

using SimpleModelLoader = ModelLoader<SimpleVertex, uint16_t>;
using PBRModelLoader = ModelLoader<PBRVertex, uint16_t>;
using SkinnedModelLoader = ModelLoader<SkinnedVertex, uint16_t>;

template <typename V, typename I> inline void ModelLoader<V, I>::InitializeGPU(ID3D12GraphicsCommandList* commandList)
{
    if (!m_world)
        return;
    ID3D12Device5* device = m_world->GetDevice();
    if (!device)
        return;

    for (auto& [name, asset] : assets)
    {
        std::shared_ptr<StaticMesh> mesh = std::make_shared<StaticMesh>();
        mesh->Initialize<V, I>(device, commandList, asset.m_meshes);
        m_world->AddMesh(name, mesh);
    }
}

template <typename V, typename I>
inline std::vector<Mesh<V, I>> ModelLoader<V, I>::GetAsset(const std::string& assetName) const
{
    auto it = assets.find(assetName);
    if (it == assets.end())
        return std::vector<Mesh<V, I>>();

    return it->second.m_meshes;
}

template <typename V, typename I> inline DirectX::SimpleMath::Vector3 ModelLoader<V, I>::aiToVector3(aiVector3D vector)
{
    DirectX::SimpleMath::Vector3 v(vector.x, vector.y, vector.z);
    return v;
}

template <typename V, typename I>
inline SkinnedLocalConstant ModelLoader<V, I>::GetCurrentSLC(const float& frame, const std::string& assetName,
                                                             const int& clipIdx, bool updateRootPos)
{
    SkinnedLocalConstant slc;

    if (assets.find(assetName) == assets.end())
    {
        return slc;
    }
    if (assets[assetName].clips.size() <= (size_t)clipIdx)
    {
        // std::cout << "SkinnedLocalConstant ModelLoader<SkinnedVertex, uint16_t>::GetCurrentSLC() clip empty\n";
        return slc;
    }
    Asset<V, I>& asset = assets[assetName];
    return asset.Update(frame, clipIdx, updateRootPos);
}

template <typename V, typename I>
inline void ModelLoader<V, I>::UpdateSLC(const float& frame, const std::string& assetName,
                                         const int& clipIdx, bool updateRootPos, ConstantBuffer<SkinnedLocalConstant> & cb)
{

    if (assets.find(assetName) == assets.end())
    {
        return;
    }
    if (assets[assetName].clips.size() <= (size_t)clipIdx)
    {
        // std::cout << "SkinnedLocalConstant ModelLoader<SkinnedVertex, uint16_t>::GetCurrentSLC() clip empty\n";
        return;
    }
    Asset<V, I>& asset = assets[assetName];
    cb.localConstant = asset.Update(frame, clipIdx, updateRootPos);
    cb.Update();
}

template <typename V, typename I> inline SkinnedLocalConstant ModelLoader<V, I>::BlendPose(const BlendData& blendData)
{
    SkinnedLocalConstant slc;

    if (assets.find(blendData.animName0) == assets.end() || assets.find(blendData.animName1) == assets.end())
    {
        return slc;
    }
    if (assets[blendData.animName0].clips.size() <= (size_t)blendData.clipId0 ||
        assets[blendData.animName1].clips.size() <= (size_t)blendData.clipId1)
    {
        // std::cout << "SkinnedLocalConstant ModelLoader<SkinnedVertex, uint16_t>::GetCurrentSLC() clip empty\n";
        return slc;
    }
    Asset<V, I>& A = assets[blendData.animName0];
    Asset<V, I>& B = assets[blendData.animName1];

    const AnimationClip& clipA = A.clips[blendData.clipId0];
    const AnimationClip& clipB = B.clips[blendData.clipId1];

    int frame0 = (int)(blendData.frame0);
    int frame1 = frame0 + 1;
    float alpha = blendData.frame0 - float(frame0);

    A.animData.accumulatedRootTransform = Matrix();
    B.animData.accumulatedRootTransform = Matrix();

    for (uint32_t boneId = 0; boneId < A.animData.boneIdToName.size(); boneId++)
    {
        const std::vector<AnimationKey>& keysA = clipA.keys[boneId];
        const std::vector<AnimationKey>& keysB = clipB.keys[boneId];

        const int parentIdxA = A.animData.boneParents[boneId];

        const Matrix parentMatrix =
            parentIdxA >= 0 ? A.animData.boneTransform[parentIdxA] : A.animData.accumulatedRootTransform;

        AnimationKey key = keysA.size() > 0 ? keysA[frame0 % keysA.size()] : AnimationKey();

        AnimationKey nextKey = keysA.size() > 0 ? keysA[frame1 % keysA.size()] : AnimationKey();

        AnimationKey key1 = keysB.size() > 0 ? keysB[(int)blendData.frame1 % keysB.size()] : AnimationKey();

        if (parentIdxA < 0)
        {
            key.pos.x = key.pos.z = 0.f;
        }
        XMVECTOR t = XMVectorLerp(key.pos, nextKey.pos, alpha);
        XMVECTOR s = XMVectorLerp(key.scale, nextKey.scale, alpha);
        XMVECTOR r = XMQuaternionNormalize(XMQuaternionSlerp(key.quat, nextKey.quat, alpha));

        t = XMVectorLerp(t, key1.pos, blendData.weight);
        s = XMVectorLerp(s, key1.scale, blendData.weight);
        r = XMQuaternionNormalize(XMQuaternionSlerp(r, key1.quat, blendData.weight));

        // XMVECTOR t = XMVectorLerp(key.pos, key1.pos, blendData.weight);
        // XMVECTOR s = XMVectorLerp(key.scale, key1.scale, blendData.weight);
        // XMVECTOR r = XMQuaternionNormalize(XMQuaternionSlerp(key.quat, key1.quat, blendData.weight));

        Matrix keyMat = Matrix::CreateScale(s) * Matrix::CreateFromQuaternion(r) * Matrix::CreateTranslation(t);

        A.animData.boneTransform[boneId] = keyMat * parentMatrix;
    }
    for (uint32_t i = 0; i < A.animData.boneIdToName.size(); i++)
    {
        slc.boneTransform[i] = (A.animData.defaultInvTransform * A.animData.boneOffset[i] *
                                A.animData.boneTransform[i] * A.animData.defaultTransform)
                                   .Transpose();
    }
    return slc;
}

template <typename V, typename I>
inline size_t ModelLoader<V, I>::GetAnimationSize(const std::string& assetName, const int& clipIdx) const
{
    auto it = assets.find(assetName);

    if (it == assets.end())
    {
        return 0;
    }
    if (it->second.clips.size() <= (size_t)clipIdx)

        return it->second.clips[clipIdx].keys.size();
}
template <typename V, typename I>
inline DirectX::SimpleMath::Matrix ModelLoader<V, I>::GetBoneTransform(const std::string& assetName,
                                                                       const std::string& boneName,
                                                                       const Matrix& socketTransform,
                                                                       const int& clipIdx)
{
    Matrix m;

    if (assets.find(assetName) == assets.end())
    {
        return m;
    }
    if (assets[assetName].clips.size() <= (size_t)clipIdx)
    {
        // std::cout << "SkinnedLocalConstant ModelLoader<SkinnedVertex, uint16_t>::GetCurrentSLC() clip empty\n";
        return m;
    }
    Asset<V, I>& asset = assets[assetName];
    return asset.GetBoneTransform(boneName, socketTransform, clipIdx);
}
template <typename V, typename I>
inline DirectX::SimpleMath::Quaternion ModelLoader<V, I>::aiToQuaternion(aiQuaternion quat)
{
    DirectX::SimpleMath::Quaternion q(quat.x, quat.y, quat.z, quat.w);
    return q;
}

template <typename V, typename I> inline DirectX::SimpleMath::Vector4 ModelLoader<V, I>::aiToVector4(aiColor4D vector)
{
    DirectX::SimpleMath::Vector4 v(vector.r, vector.g, vector.b, vector.a);
    return v;
}

template <typename V, typename I> inline aiMatrix4x4 ModelLoader<V, I>::MatrixToAi(const DirectX::SimpleMath::Matrix& m)
{
    return aiMatrix4x4(m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24, m._31, m._32, m._33, m._34, m._41, m._42,
                       m._43, m._44);
}

template <typename V, typename I> inline DirectX::SimpleMath::Matrix ModelLoader<V, I>::aiToMatrix(const aiMatrix4x4& m)
{
    return DirectX::SimpleMath::Matrix(m.a1, m.a2, m.a3, m.a4, m.b1, m.b2, m.b3, m.b4, m.c1, m.c2, m.c3, m.c4, m.d1,
                                       m.d2, m.d3, m.d4);
}

template <typename V, typename I>
inline void ModelLoader<V, I>::Load(std::string filename, DirectX::SimpleMath::Matrix tr, bool loadAnimation)
{
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(basePath + filename, aiProcess_ConvertToLeftHanded | aiProcess_Triangulate);

    if (scene == nullptr)
    {
        return;
    }
    Asset<V, I> asset;

    if (loadAnimation)
    {
        asset.animData.defaultTransform = tr;
        asset.animData.defaultInvTransform = tr.Invert();
        for (size_t i = 0; i < scene->mNumMeshes; i++)
        {
            const auto* mesh = scene->mMeshes[i];
            if (mesh->HasBones())
            {
				// mesh가 bone을 가지고 있다면 모든 bone에 대해
				// boneNameToId map을 -1로 초기화
                for (size_t j = 0; j < mesh->mNumBones; j++)
                {
                    const aiBone* bone = mesh->mBones[j];
                    std::string name = bone->mName.C_Str();
                    asset.animData.boneNameToId[name] = -1;
                }
            }
        }

		// bone을 root부터 내려가면서 트리 구조로 boneNameToId 채운 뒤
		// boneIdToName 초기화
        uint32_t count = 0;
        CreateBoneTree(asset, scene->mRootNode, &count);
        for (auto& i : asset.animData.boneNameToId)
            asset.animData.boneIdToName[i.second] = i.first;

        asset.animData.boneParents.resize(asset.animData.boneNameToId.size(), -1);
    }

    ProcessNode(asset, scene->mRootNode, scene, tr, loadAnimation);

    if (loadAnimation)
    {
        for (uint32_t i = 0; i < scene->mNumAnimations; i++)
        {
            const aiAnimation* ani = scene->mAnimations[i];

            AnimationClip clip;
            clip.duration = (float)ani->mDuration;
            clip.tickPerSec = (float)ani->mTicksPerSecond;
            clip.keys.resize(asset.animData.boneNameToId.size());
            clip.numChannels = ani->mNumChannels;

            // i번쨰 노드(채널 / 메쉬)
            for (uint32_t c = 0; c < ani->mNumChannels; c++)
            {
                const aiNodeAnim* nodeAnim = ani->mChannels[c];
                auto it = asset.animData.boneNameToId.find(nodeAnim->mNodeName.C_Str());
                if (it == asset.animData.boneNameToId.end())
                {
                    continue;
                }
                uint32_t boneId = it->second;
                clip.keys[boneId].resize(nodeAnim->mNumPositionKeys);

                for (size_t k = 0; k < nodeAnim->mNumPositionKeys; k++)
                {
                    clip.keys[boneId][k].pos = aiToVector3(nodeAnim->mPositionKeys[k].mValue);
                    clip.keys[boneId][k].quat = aiToQuaternion(nodeAnim->mRotationKeys[k].mValue);
                    clip.keys[boneId][k].scale = aiToVector3(nodeAnim->mScalingKeys[k].mValue);
                }
            }
            asset.clips.push_back(clip);
        }
    }
    std::filesystem::path p = filename;
    assets[p.stem().string()] = asset;
}

template <typename V, typename I>
inline void ModelLoader<V, I>::LoadAnimations(std::string folderPath, std::string targetAssetName)
{
    namespace fs = std::filesystem;

    fs::path dirPath = fs::path(basePath + folderPath);

    std::vector<std::string> files;
    for (const auto& e : fs::directory_iterator(dirPath))
    {
        if (e.is_regular_file())
            files.push_back(e.path().string());
    }

    // omp_set_num_threads(4);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(files.size()); ++i)
    {
        LoadAnimation(files[i], targetAssetName, false);
    }
}

template <typename V, typename I>
inline void ModelLoader<V, I>::LoadAnimation(std::string filePath, std::string targetAssetName, bool useBasePath)
{
    Assimp::Importer importer;
    std::string path = useBasePath ? (basePath + filePath) : filePath;
    const aiScene* scene = importer.ReadFile(path, aiProcess_ConvertToLeftHanded | aiProcess_Triangulate);

    if (scene == nullptr)
    {
        return;
    }
    Asset<V, I> asset;
    auto targetAsset = assets.find(targetAssetName);
    if (targetAsset == assets.end())
    {
        std::cout << "Failed to load animation - " << filePath << '\n';
        return;
    }
    asset.animData = targetAsset->second.animData;

    for (uint32_t i = 0; i < scene->mNumAnimations; i++)
    {
        const aiAnimation* ani = scene->mAnimations[i];

        AnimationClip clip;
        clip.duration = (float)ani->mDuration;
        clip.tickPerSec = (float)ani->mTicksPerSecond;
        clip.keys.resize(asset.animData.boneNameToId.size());
        clip.numChannels = ani->mNumChannels;

        for (int c = 0; c < ani->mNumChannels; c++)
        {
            const aiNodeAnim* nodeAnim = ani->mChannels[c];
            auto it = asset.animData.boneNameToId.find(nodeAnim->mNodeName.C_Str());
            if (it == asset.animData.boneNameToId.end())
            {
                // std::cout << "hi ";
                continue;
            }
            uint32_t boneId = it->second;
            clip.keys[boneId].resize(nodeAnim->mNumPositionKeys);

            for (int k = 0; k < nodeAnim->mNumPositionKeys; k++)
            {
                clip.keys[boneId][k].pos = aiToVector3(nodeAnim->mPositionKeys[k].mValue);
                clip.keys[boneId][k].quat = aiToQuaternion(nodeAnim->mRotationKeys[k].mValue);
                clip.keys[boneId][k].scale = aiToVector3(nodeAnim->mScalingKeys[k].mValue);
            }
        }
        asset.clips.push_back(clip);
    }
    std::filesystem::path p = filePath;
    assets[p.stem().string()] = asset;
}

template <typename V, typename I> const aiNode* ModelLoader<V, I>::FindParent(Asset<V, I>& asset, const aiNode* node)
{
    if (!node)
        return nullptr;
    auto it = asset.animData.boneNameToId.find(node->mName.C_Str());
    if (it == asset.animData.boneNameToId.end())
    {
        return FindParent(asset, node->mParent);
    }
    else
        return node;
}

template <typename V, typename I>
inline void ModelLoader<V, I>::ProcessNode(Asset<V, I>& asset, aiNode* node, const aiScene* scene,
                                           DirectX::SimpleMath::Matrix tr, bool loadAnimation)
{
    auto& boneNameToId = asset.animData.boneNameToId;
    if (node->mParent && boneNameToId.count(node->mName.C_Str()))
    {
        auto parentsNode = FindParent(asset, node->mParent);
        if (parentsNode != nullptr)
        {
            const auto boneId = boneNameToId[node->mName.C_Str()];
            asset.animData.boneParents[boneId] = boneNameToId[parentsNode->mName.C_Str()];
        }
    }
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(asset, mesh, scene, tr, loadAnimation);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(asset, node->mChildren[i], scene, tr, loadAnimation);
    }
}

template <typename V, typename I>
inline void ModelLoader<V, I>::LoadPointCloud(std::string filename, DirectX::SimpleMath::Matrix tr)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(basePath + filename, 0);
    if (scene == nullptr)
    {
        return;
    }
    Asset<V, I> asset;
    ProcessPCNode(asset.m_meshes, scene->mRootNode, scene, tr);
    std::filesystem::path p = filename;
    assets[p.stem().string()] = asset;
}

template <typename V, typename I>
inline void ModelLoader<V, I>::ProcessPCNode(std::vector<Mesh<V, I>>& meshes, aiNode* node, const aiScene* scene,
                                             DirectX::SimpleMath::Matrix tr)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessPCMesh(meshes, mesh, scene, tr);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessPCNode(meshes, node->mChildren[i], scene, tr);
    }
}

template <typename V, typename I>
inline void ModelLoader<V, I>::ProcessPCMesh(std::vector<Mesh<V, I>>& meshes, aiMesh* mesh, const aiScene* scene,
                                             DirectX::SimpleMath::Matrix tr)
{
    Mesh<V, I> meshData;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        aiColor4D c(1.f, 1.f, 1.f, 1.f);

        if (mesh->HasVertexColors(0) && mesh->mColors[0])
            c = mesh->mColors[0][i];
        Vector3 pos = aiToVector3(v);
        pos = Vector3::Transform(pos, tr);

        meshData.m_vertices.push_back(Vertex(pos, aiToVector4(c)));
    }
    meshes.push_back(meshData);
}

template <typename V, typename I>
inline void ModelLoader<V, I>::CreateBoneTree(Asset<V, I>& asset, aiNode* node, uint32_t* count)
{
    if (node)
    {
        auto& animData = asset.animData;
        auto it = animData.boneNameToId.find(node->mName.C_Str());
        if (it != animData.boneNameToId.end())
        {
            animData.boneNameToId[node->mName.C_Str()] = *count;
            (*count) += 1;
        }
        for (size_t i = 0; i < node->mNumChildren; i++)
        {
            CreateBoneTree(asset, node->mChildren[i], count);
        }
    }
}

template <typename V, typename I>
inline void ModelLoader<V, I>::GetBonePosition(Asset<V, I>& asset, aiNode* node, const aiMatrix4x4& parentTransform,
                                               const aiScene* scene)
{
    aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
    auto& animData = asset.animData;

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        if (mesh->HasBones())
        {
            for (size_t j = 0; j < mesh->mNumBones; j++)
            {
                aiBone* bone = mesh->mBones[j];
                auto boneOffset = bone->mOffsetMatrix;
                auto bonePosition = nodeTransform * boneOffset;

                DirectX::SimpleMath::Matrix m = aiToMatrix(bonePosition);
                uint32_t boneId = asset.animData.boneNameToId[bone->mName.C_Str()];
                animData.bonePosition[boneId] = m.Translation();
            }
        }
    }

    for (size_t c = 0; c < node->mNumChildren; c++)
    {
        GetBonePosition(asset, node->mChildren[c], nodeTransform, scene);
    }
}
