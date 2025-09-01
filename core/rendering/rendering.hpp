#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <string>
#include <dwmapi.h>

#include <core/types/vector.hpp>

struct ImDrawData;
struct ImGuiContext;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")

using Microsoft::WRL::ComPtr;

class DX11Renderer
{
public:
    DX11Renderer();
    ~DX11Renderer();

    bool initialize(const char *windowTitle, bool borderlessFullscreen = true, const char *targetWindowTitle = nullptr, const char *targetWindowClass = nullptr);

    void shutdown();

    bool initializeImGui();
    void shutdownImGui();

    void beginImGuiFrame() const;
    void endImGuiFrame() const;
    void renderImGui() const;

    void beginFrame(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void endFrame();

    void onResize(Vector2<LONG> size);

    bool processMessages();

    HWND getWindow() const { return m_hwnd; }
    ID3D11Device *getDevice() const { return m_device.Get(); }
    ID3D11DeviceContext *getContext() const { return m_context.Get(); }
    ID3D11RenderTargetView *getRenderTargetView() const { return m_renderTargetView.Get(); }
    ID3D11DepthStencilView *getDepthStencilView() const { return m_depthStencilView.Get(); }

    Vector2<LONG> getSize() const { return m_size; }

    bool isInitialized() const { return m_initialized; }
    bool isVSyncEnabled() const { return m_vsyncEnabled; }
    void setVSync(bool enabled) { m_vsyncEnabled = enabled; }

    bool isBorderlessFullscreen() const { return m_borderlessFullscreen; }
    void setBorderlessFullscreen(bool enabled);

    void updateOverlayPosition();
    void setOverlayInteractive(bool interactive) const;
    HWND getTargetWindow() const { return m_targetHwnd; }

private:
    bool createDevice();
    bool createSwapChain(HWND hwnd);
    bool createRenderTargetView();
    bool createDepthStencilBuffer();
    bool createRasterizerState();
    bool createBlendState();
    void setViewport();

    void releaseRenderTargets();

    HWND createWindow(const char *title, Vector2<LONG> size);
    HWND createOverlayWindow();
    void makeWindowTransparent(HWND hwnd) const;
    HWND findTargetWindow(const char *windowTitle, const char *windowClass);
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
    Vector2<LONG> m_size;
    bool m_borderlessFullscreen;
    bool m_vsyncEnabled;
    bool m_initialized;
    bool m_imguiInitialized;
    bool m_shouldClose;

    D3D_FEATURE_LEVEL m_featureLevel;
    DXGI_FORMAT m_backBufferFormat;
    UINT m_msaaQuality;
    UINT m_sampleCount;

    HWND m_targetHwnd;
    RECT m_targetRect;
    MARGINS m_margins;
};
