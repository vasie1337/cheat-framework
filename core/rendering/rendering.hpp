#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

class DX11Renderer
{
public:
    DX11Renderer();
    ~DX11Renderer();

    bool initialize(HWND hwnd, int width, int height, bool fullscreen = false);
    bool initialize(const char* windowTitle, int width, int height, bool fullscreen = false);
    
    void shutdown();
    
    void beginFrame(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void endFrame();
    
    void onResize(int width, int height);
    
    bool processMessages();
    
    HWND getWindow() const { return m_hwnd; }
    ID3D11Device* getDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* getContext() const { return m_context.Get(); }
    ID3D11RenderTargetView* getRenderTargetView() const { return m_renderTargetView.Get(); }
    ID3D11DepthStencilView* getDepthStencilView() const { return m_depthStencilView.Get(); }
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    bool isInitialized() const { return m_initialized; }
    bool isVSyncEnabled() const { return m_vsyncEnabled; }
    void setVSync(bool enabled) { m_vsyncEnabled = enabled; }

private:
    bool createDevice();
    bool createSwapChain(HWND hwnd);
    bool createRenderTargetView();
    bool createDepthStencilBuffer();
    bool createRasterizerState();
    bool createBlendState();
    void setViewport();
    
    void releaseRenderTargets();
    
    HWND createWindow(const char* title, int width, int height);
    static LRESULT CALLBACK windowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<IDXGIFactory2> m_dxgiFactory;
    
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    
    ComPtr<ID3D11RasterizerState> m_rasterizerState;
    ComPtr<ID3D11BlendState> m_blendState;
    
    HWND m_hwnd;
    bool m_ownsWindow;
    int m_width;
    int m_height;
    bool m_fullscreen;
    bool m_vsyncEnabled;
    bool m_initialized;
    bool m_shouldClose;
    
    D3D_FEATURE_LEVEL m_featureLevel;
    DXGI_FORMAT m_backBufferFormat;
    UINT m_msaaQuality;
    UINT m_sampleCount;
};
