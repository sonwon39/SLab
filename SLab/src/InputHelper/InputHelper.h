#pragma once

#include <map>
#include "directxtk12\SimpleMath.h"

class Actor;

class InputHelper
{
  public:
    InputHelper();
    virtual ~InputHelper();

  public:
    void Initialize();

    void SetKeyState(uint32_t key, bool newState);
    bool GetKeyState(uint32_t key);

    bool GetCaptureFlag() const
    {
        return captureFlag;
    }
    bool GetRecordFlag() const
    {
        return recordFlag;
    }
    void SetCaptureFlag(bool state)
    {
        captureFlag = state;
    }
    void SetRecordFlag(bool state)
    {
        recordFlag = state;
    }

	bool captureFlag = false;
    bool recordFlag = false;

    DirectX::SimpleMath::Vector3 GetInputDirection(Actor* actor);

  private:
    std::map<uint32_t, bool> m_keyDownState;

  private:
    uint32_t forwardKey = 'W';
    uint32_t backwardKey = 'S';
    uint32_t rightKey = 'D';
    uint32_t leftKey = 'A';
    uint32_t upKey = 'E';
    uint32_t downKey = 'Q';
    uint32_t captureKey = 'C';
    uint32_t recordKey = 'R';
};
