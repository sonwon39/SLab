#include "BaseApp.h"
#include "GraphicsCommon.h"
#include "Engine/World.h"

using namespace Core;
using namespace GraphicsUtils;
using namespace Graphics;

BaseApp* BaseApp::m_appPtr = nullptr;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return BaseApp::m_appPtr->MainProc(hWnd, msg, wParam, lParam);
}

BaseApp::BaseApp() : m_width(1280), m_height(720)
{
    if (m_appPtr != nullptr)
    {
        delete m_appPtr;
    }
    // LoadLibraryA("torch_cuda.dll");
    m_appPtr = this;
    m_timer = Timer();
    Graphics::m_world = std::make_unique<World>();
}

BaseApp::BaseApp(int width, int height) : m_width(width), m_height(height)
{
    if (m_appPtr != nullptr)
    {
        delete m_appPtr;
    }
    m_appPtr = this;
    m_timer = Timer();
    Graphics::m_world = std::make_unique<World>();
}

BaseApp::~BaseApp()
{
    /*ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();*/

    if (m_world)
        m_world->m_mainWnd = NULL;

    m_appPtr = nullptr;
    std::cout << "Delete BaseApp\n";
}

bool BaseApp::Initialize()
{
    if (!InitWindow())
    {
        std::cout << "InitWindow Failed\n";
        return false;
    }

    if (!InitDirectX())
    {
        std::cout << "InitDirectX Failed\n";
        return false;
    }

    // Close commandList & Flush Queue
    // OnResize();

    if (!InitGUI())
    {
        std::cout << "InitGUI Failed\n";
        return false;
    }

    return true;
}

bool BaseApp::InitWindow()
{
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"D3DApp";
    wc.hIconSm = NULL;

    RegisterClassEx(&wc);

    // Create window
    RECT rc = {0, 0, (LONG)m_width, (LONG)m_height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    m_world->m_mainWnd =
        CreateWindow(wc.lpszClassName, wc.lpszClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                     (int)(rc.right - rc.left), (int)(rc.bottom - rc.top), NULL, NULL, wc.hInstance, nullptr);

    RAWINPUTDEVICE rawInputDevice;

    // The HID standard for mouse
    const uint16_t standardMouse = 0x02;

    rawInputDevice.usUsagePage = 0x01;
    rawInputDevice.usUsage = standardMouse;
    rawInputDevice.dwFlags = 0;
    rawInputDevice.hwndTarget = m_world->m_mainWnd;

    ::RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE));

    ShowWindow(m_world->m_mainWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_world->m_mainWnd);

    // ShowCursor(false);

    return true;
}

bool Core::BaseApp::IsWindowFocused()
{
    return (GetForegroundWindow() == m_world->m_mainWnd);
}

LRESULT BaseApp::MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_SIZE:
    {
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);
        OnResize();

        return 0;
    }
    break;
    case WM_INPUT:
    {
        RAWINPUT raw;
        UINT rawSize = sizeof(raw);

        const UINT resultData =
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));

        if (m_world && m_world->mouse && m_world->GetFPSMode())
        {
            m_world->mouse->AddRawDelta(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
        }
    }
    break;
    case WM_LBUTTONDOWN:
        if (m_world && m_world->mouse)
        {
            m_world->mouse->UpdateLButtonDownState(true);
        }
        break;
    case WM_LBUTTONUP:
        if (m_world && m_world->mouse)
        {
            m_world->mouse->UpdateLButtonDownState(false);
        }
        break;
    case WM_KEYDOWN:
        if (m_world && m_world->m_inputHelper)
        {
            m_world->m_inputHelper->SetKeyState((uint32_t)wParam, true);
        }
        break;
    case WM_KEYUP:
        if (m_world && m_world->m_inputHelper)
        {
            m_world->m_inputHelper->SetKeyState((uint32_t)wParam, false);
        }
        break;
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
        if (m_world)
        {
            bool currState = m_world->GetFPSMode();
            m_world->SetFPSMode(!currState);
        }
        break;
    case WM_XBUTTONUP:

    case WM_MOUSEACTIVATE:
        // When you click to activate the window, we want Mouse to ignore that event.
        return MA_ACTIVATEANDEAT;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
