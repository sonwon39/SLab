#pragma once

class Actor;

// 모든 컴포넌트의 베이스. 자신을 소유한 Actor 포인터와 Tick 인터페이스만 제공한다.
// (트랜스폼/계층 기능은 파생 클래스 SceneComponent에서 추가됨)
class ActorComponent
{
  public:
    ActorComponent(Actor* owner);
    virtual ~ActorComponent();

  public:
    virtual void Initialize() {};
    void SetComponentTickEnabled(bool enabled)
    {
        bCanTick = enabled;
    }
    bool GetComponentTickEnabled()
    {
        return bCanTick;
    }

  public:
    Actor* GetOwner()
    {
        return m_owner;
    }

  public:
    virtual void TickComponent(const float& deltaTime);

  protected:
    Actor* m_owner;
    bool bCanTick = false;
};
