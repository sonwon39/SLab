#pragma once

#include <iostream>
#include <atomic>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "Core/Timer.h"

namespace Core
{
class BaseApp
{

  public:
    BaseApp();
    BaseApp(int width, int height);
    virtual ~BaseApp();

    virtual bool Initialize();
    LRESULT MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual int Run() = 0;

  private:
    virtual bool InitDirectX() = 0;
    virtual bool InitGUI() = 0;

    // Called when the window is resized
    virtual void OnResize() = 0;
    virtual void OnCapture() {};
    bool InitWindow();

  protected:
    bool IsWindowFocused();

  public:
    static BaseApp* m_appPtr;
    Core::Timer m_timer;

  protected:
    int m_width;
    int m_height;

    bool isFPSMode = true;

    bool captureDirty = false;
};
} // namespace Core
