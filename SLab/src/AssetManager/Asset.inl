#pragma once

#include "Asset.h"

template <typename V, typename I>
inline SkinnedLocalConstant Asset<V, I>::Update(const float& frame, const int& clipIdx, bool updateRootPos)
{
    SkinnedLocalConstant slc;

    const AnimationClip& clip = clips[clipIdx];

    int currFrame = (int)(frame);

    int frame0 = (int)(frame);
    int frame1 = frame0 + 1;
    float alpha = frame - float(frame0);

    animData.accumulatedRootTransform = Matrix();
    for (uint32_t boneId = 0; boneId < animData.boneIdToName.size(); boneId++)
    {
        const std::vector<AnimationKey>& keys = clip.keys[boneId];

        const int parentIdx = animData.boneParents[boneId];
        const Matrix parentMatrix =
            parentIdx >= 0 ? animData.boneTransform[parentIdx] : animData.accumulatedRootTransform;

        AnimationKey key = keys.size() > 0 ? keys[frame0 % keys.size()] : AnimationKey();

        AnimationKey nextKey = keys.size() > 0 ? keys[frame1 % keys.size()] : AnimationKey();

        if (parentIdx < 0 && !updateRootPos)
        {
            key.pos.x = key.pos.z = 0.f;
        }
        XMVECTOR t = XMVectorLerp(key.pos, nextKey.pos, alpha);
        XMVECTOR s = XMVectorLerp(key.scale, nextKey.scale, alpha);
        XMVECTOR r = XMQuaternionNormalize(XMQuaternionSlerp(key.quat, nextKey.quat, alpha));

        Matrix keyMat = Matrix::CreateScale(s) * Matrix::CreateFromQuaternion(r) * Matrix::CreateTranslation(t);

        animData.boneTransform[boneId] = keyMat * parentMatrix;
    }
    for (uint32_t i = 0; i < animData.boneIdToName.size(); i++)
    {
        slc.boneTransform[i] = (animData.defaultInvTransform * animData.boneOffset[i] * animData.boneTransform[i] *
                                animData.defaultTransform)
                                   .Transpose();
    }
    return slc;
}

template <typename V, typename I>
inline Matrix Asset<V, I>::GetBoneTransform(const std::string& boneName, const Matrix& socketTransform,
                                            const int& clipIdx)
{
    auto it = animData.boneNameToId.find(boneName);
    if (it == animData.boneNameToId.end())
        return Matrix();

    int32_t i = it->second;

    return (animData.defaultInvTransform * socketTransform * animData.boneTransform[i] * animData.defaultTransform);
}
