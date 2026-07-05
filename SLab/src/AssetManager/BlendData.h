#pragma once

#include <string>

struct BlendData
{
    float frame0;
    float frame1;
    std::string animName0;
    std::string animName1;
    float weight;
    int clipId0;
    int clipId1;
};
