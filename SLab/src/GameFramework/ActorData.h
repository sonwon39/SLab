#pragma once

#include <string>
#include <directxtk12/SimpleMath.h>
#include "PBR/PBRHLSLCompat.h"
#include "PhysXMode.h"

enum ActorState
{
    AS_default,
    AS_idle,
    AS_attack,
    AS_playMontage
};

struct ActorData
{
    ActorData()
    {
        useSimulate = false;
        mode = PhysXMode::PM_Default;
        updateConstants = false;

        mc.heightScale = 0.f;
        mc.roughness = 0.2f;
        mc.metallic = 0.8f;
        useMaterial = false;
        collisionLocation = Vector3(0.f, 0.f, 0.f);
    };

    std::string name;
    std::string mesh;

    std::string textureName;
    std::string psoName;

    bool useSimulate;
    PhysXMode mode = PhysXMode::PM_Default;

    bool updateConstants;
    Vector3 collisionLocation;

    LocalConstant lc;
    bool useMaterial;
    MaterialConstant mc{};
};

struct LightData
{
    float viewWidth;
    float viewHeight;
    float nearZ;
    float farZ;

    float intensity;
    Vector4 brightness;
    Vector4 color;
    Vector3 dir;
};

struct AnimData
{
    std::string name;
    float animationSpeed = 60.f;
    bool playAnimation = true;
    ActorState actorState = ActorState::AS_default;
};
