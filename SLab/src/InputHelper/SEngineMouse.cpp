#include "SEngineMouse.h"
#include "GraphicsCommon.h"
#include "Engine/World.h"

#include <iostream>
#include <algorithm>

using namespace Graphics;
using namespace DirectX::SimpleMath;

SEngineMouse::SEngineMouse()
{
}

SEngineMouse::~SEngineMouse()
{
}

void SEngineMouse::Initilize()
{
    mouseCB.Initialize({});
}

void SEngineMouse::UpdateLButtonDownState(bool newState)
{
    lButtonDown = newState;
    mouseCB.localConstant.lButtonDown = newState;

    if (newState)
    {
        lBFlag = true;
    }
    // std::cout << "UpdateLButtonDownState " << newState << '\n';
}

void SEngineMouse::Tick(float deltaTime)
{
    GetCursorPos(&mousePos);
    ScreenToClient(m_world->m_mainWnd, &mousePos);

	StableFluidsMouseTick(deltaTime);

	if (m_world->GetFPSMode())
	{
        m_world->MoveMouseToWindowCenter();
	}
}

void SEngineMouse::StableFluidsMouseTick(float deltaTime)
{
    static int i = 0;
	if (i == 0)
	{
        mouseCB.localConstant.color = m_world->colors[0];
	}

    int width = m_world->windowWidth;
    int height = m_world->windowHeight;

    float ndcX = (mousePos.x * 2.f) / width - 1.f;
    float ndcY = (-mousePos.y * 2.f) / height + 1.f;

    ndcX = std::clamp(ndcX, -1.0f, 1.0f);
    ndcY = std::clamp(ndcY, -1.0f, 1.0f);

    currNDCPos = Vector2(ndcX, -ndcY);

    mouseCB.localConstant.posX = mousePos.x;
    mouseCB.localConstant.posY = mousePos.y;

    if (lBFlag)
    {
        i++;
        int index = i % m_world->colors.size();
        lBFlag = false;
        prevNDCPos = currNDCPos;
        mouseCB.localConstant.color = m_world->colors[index];
    }

    mouseCB.localConstant.velocity = (currNDCPos - prevNDCPos) * 10.f;

    mouseCB.Update();

    prevNDCPos = currNDCPos;
}
