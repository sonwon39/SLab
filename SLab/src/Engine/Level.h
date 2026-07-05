#pragma once

#include "GameFramework\Actor.h"
#include <vector>

class Level
{
  public:
    Level();
    virtual ~Level();

  public:
    void Initialize();

  private:
    std::vector<Actor> m_actors;
};
