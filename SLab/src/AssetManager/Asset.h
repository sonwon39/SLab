#pragma once

#include "directxtk12/SimpleMath.h"
#include <vector>
#include <unordered_map>
#include <string>
#include "Mesh.h"
#include "PBR/PBRHLSLCompat.h"

struct AnimationKey
{
    DirectX::SimpleMath::Vector3 pos;
    DirectX::SimpleMath::Vector3 scale;
    DirectX::SimpleMath::Quaternion quat;
};

struct AnimationClip
{
    float duration;
    float tickPerSec;
    std::vector<std::vector<AnimationKey>> keys;
    uint32_t numChannels;
};

struct AnimationData
{
    std::unordered_map<std::string, int32_t> boneNameToId;
    std::vector<int32_t> boneParents;
    std::unordered_map<int32_t, std::string> boneIdToName;
    std::unordered_map<int32_t, DirectX::SimpleMath::Vector3> bonePosition;
    std::vector<DirectX::SimpleMath::Matrix> boneOffset;
    std::vector<DirectX::SimpleMath::Matrix> boneTransform;
    DirectX::SimpleMath::Matrix defaultTransform;
    DirectX::SimpleMath::Matrix defaultInvTransform;
    Matrix accumulatedRootTransform = Matrix();
    Vector3 prevPos = Vector3(0.0f);
};

template <typename V, typename I> class Asset
{
  public:
    Asset() {};
    virtual ~Asset() {};

    std::vector<Mesh<V, I>> m_meshes;
    AnimationData animData;
    std::vector<AnimationClip> clips;
    std::string name;

    SkinnedLocalConstant Update(const float& frame, const int& clipIdx, bool updateRootPos);
    Matrix GetBoneTransform(const std::string& boneName, const Matrix& socketTransform, const int& clipIdx);
};
#include "Asset.inl"
