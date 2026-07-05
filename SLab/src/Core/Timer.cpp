#include "Timer.h"

Core::Timer::Timer()
    : m_DeltaTime(-1.f), m_SecondPerTick(0), m_BaseTime(0), m_CurrTime(0), m_PrevTime(0), m_StartTime(0), m_StopTime(0),
      m_PausedTime(0), m_Stopped(false)
{
    __int64 tickPerSecond;
    QueryPerformanceFrequency((LARGE_INTEGER*)&tickPerSecond);

    m_SecondPerTick = 1.0 / tickPerSecond;
}

Core::Timer::~Timer()
{
}

void Core::Timer::Start()
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    if (m_Stopped)
    {
        m_Stopped = false;
        m_PrevTime = currTime;
        m_Stopped = 0;
        m_PausedTime += currTime - m_StopTime;
    }
}

void Core::Timer::Stop()
{
    if (!m_Stopped)
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        m_Stopped = true;
        m_StopTime = currTime;
    }
}

void Core::Timer::Tick()
{
    if (m_Stopped)
    {
        m_DeltaTime = 0.0;
        return;
    }
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    m_CurrTime = currTime;
    m_DeltaTime = (currTime - m_PrevTime) * m_SecondPerTick;
    m_PrevTime = currTime;

    if (m_DeltaTime < 0)
    {
        m_DeltaTime = 0.0;
    }
}

void Core::Timer::Reset()
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    m_BaseTime = currTime;
    m_PrevTime = currTime;
    m_StopTime = 0;
    m_PausedTime = 0;
    m_Stopped = false;
}

double Core::Timer::GetDeltaTime() const
{
    return m_DeltaTime;
}

double Core::Timer::GetElapsedTime() const
{
    if (m_Stopped)
    {

        return (m_StopTime - m_BaseTime - m_PausedTime) * m_SecondPerTick;
    }
    else
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        return (currTime - m_BaseTime - m_PausedTime) * m_SecondPerTick;
    }
}

bool Core::Timer::IsStopped() const
{
    return m_Stopped;
}
