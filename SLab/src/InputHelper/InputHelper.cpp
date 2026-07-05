#include "InputHelper.h"
#include <iostream>
#include "GameFramework\Actor.h"

using namespace DirectX::SimpleMath;

InputHelper::InputHelper()
{
}

InputHelper::~InputHelper()
{
}

void InputHelper::Initialize()
{
    for (char c = 'A'; c <= 'Z'; c++)
    {
        m_keyDownState[c] = false;
    }
}

void InputHelper::SetKeyState(uint32_t key, bool newState)
{
    m_keyDownState[key] = newState;
	if (key == captureKey && newState)
	{
        captureFlag = true;
	}
    if (key == recordKey && newState)
    {
        recordFlag = true;
    }
}

bool InputHelper::GetKeyState(uint32_t key)
{
    auto it = m_keyDownState.find(key);
    if (it != m_keyDownState.end())
        return it->second;
    return false;
}

DirectX::SimpleMath::Vector3 InputHelper::GetInputDirection(Actor* actor)
{
    Vector3 dir = Vector3(0.f, 0.f, 0.f);

    Vector3 front = actor->GetActorFrontDir();
    Vector3 back = -front;
    Vector3 right = actor->GetActorRightDir();
    Vector3 left = -right;
    Vector3 up = actor->GetActorUpDir();
    Vector3 down = -up;

    if (m_keyDownState[forwardKey])
        dir += front;

    if (m_keyDownState[backwardKey])
        dir += back;

    if (m_keyDownState[rightKey])
        dir += right;

    if (m_keyDownState[leftKey])
        dir += left;

    if (m_keyDownState[upKey])
        dir += up;

    if (m_keyDownState[downKey])
        dir += down;

    dir.Normalize();
    return dir;
}
