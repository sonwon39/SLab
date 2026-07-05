#pragma once

#include "directxtk12/SpriteBatch.h"
#include "directxtk12/SpriteFont.h"
#include "directxtk12/GraphicsMemory.h"

#include "BaseApp.h"
#include "Renderer.h"

namespace Core
{
class SimpleApp : public BaseApp
{
  public:
    SimpleApp();
    SimpleApp(const int width, const int height, const int guiWidth);
    // MultiThreadApp(const int width, const int height);

    virtual ~SimpleApp();
    virtual int Run() override;

  protected:
    virtual bool InitDirectX() override;
    virtual bool InitGUI() override;

  protected:
    void Update(float deltaTime);

    // Called when the window is resized
    void OnResize() override;

  private:
    Microsoft::WRL::ComPtr<IDXGIFactory7> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_adapter;
    Microsoft::WRL::ComPtr<ID3D12Device5> m_device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

    float deltaTime = 0.f;
    int m_guiWidth;

  private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
};
} // namespace Core
