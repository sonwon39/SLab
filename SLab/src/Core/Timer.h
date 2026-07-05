#pragma once

#define NOMINMAX
#include <Windows.h>

namespace Core
{
class Timer
{
  public:
    Timer();
    virtual ~Timer();

  public:
    void Start();
    void Stop();
    void Tick();
    void Reset();

  public:
    double GetDeltaTime() const;
    double GetElapsedTime() const;
    bool IsStopped() const;

  private:
    double m_DeltaTime;
    double m_SecondPerTick;

    __int64 m_BaseTime;
    __int64 m_CurrTime;
    __int64 m_PrevTime;
    __int64 m_StartTime;
    __int64 m_StopTime;
    __int64 m_PausedTime;

    bool m_Stopped;
};
} // namespace Core
