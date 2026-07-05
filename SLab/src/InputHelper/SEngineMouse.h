#pragma once

#include "directxtk12\SimpleMath.h"
#include "GlobalConstant.h"
#include "Core/ConstantBuffer.h"

class SEngineMouse
{
  public:
    SEngineMouse();
    virtual ~SEngineMouse();

  public:
    // device 생성 후 constant buffer 생성
    void Initilize();

    // constantbuffer 업데이트
    void Tick(float deltaTime);

    DirectX::SimpleMath::Vector2 GetMouseRawDelta() const
    {
        return rawDelta;
    }
    void AddRawDelta(long dx, long dy)
    {
        rawDelta.x += dx;
        rawDelta.y += dy;
    }
    void SetRawDelta(DirectX::SimpleMath::Vector2 delta)
    {
        rawDelta = delta;
    }
    void StableFluidsMouseTick(float deltaTime);

  public:
    void UpdateLButtonDownState(bool newState);

  public:
    ConstantBuffer<MouseConstant> mouseCB;
    POINT mousePos;

  public:
    bool lButtonDown = false;

    DirectX::SimpleMath::Vector2 currNDCPos = DirectX::SimpleMath::Vector2(0, 0);
    DirectX::SimpleMath::Vector2 prevNDCPos = DirectX::SimpleMath::Vector2(0, 0);
    DirectX::SimpleMath::Vector2 currPos = DirectX::SimpleMath::Vector2(0, 0);
    DirectX::SimpleMath::Vector2 prevPos = DirectX::SimpleMath::Vector2(0, 0);
    DirectX::SimpleMath::Vector2 velocity = DirectX::SimpleMath::Vector2(0, 0);
    // lButton 첫 클릭 여부 확인 flag
    // 첫 클릭일 시 tick에서 prevpos를 현재 위치로 업데이트
    bool lBFlag = false;

  private:
    DirectX::SimpleMath::Vector2 rawDelta = {0, 0};
};
