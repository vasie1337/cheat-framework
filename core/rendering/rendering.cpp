#include "rendering.hpp"
#include <core/logger/logger.hpp>
#include <stdexcept>
#include <cstring>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

DX11Renderer::DX11Renderer()
    : m_hwnd(nullptr)
    , m_size(0, 0)
    , m_borderlessFullscreen(false)
    , m_vsyncEnabled(false)
    , m_initialized(false)
    , m_shouldClose(false)
    , m_featureLevel(D3D_FEATURE_LEVEL_11_0)
    , m_backBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    , m_msaaQuality(0)
    , m_sampleCount(1)
    , m_targetHwnd(nullptr)
    , m_imguiInitialized(false)
{
    m_targetRect = { 0, 0, 0, 0 };
    m_margins = { -1, -1, -1, -1 };
}

DX11Renderer::~DX11Renderer()
{
    shutdown();
}

bool DX11Renderer::initialize(const char* windowTitle, bool borderlessFullscreen)
{
    if (m_initialized)
    {
        log_warning("Renderer already initialized");
        return true;
    }

    m_borderlessFullscreen = borderlessFullscreen;

    m_size = Vector2<LONG>(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    m_hwnd = createWindow(windowTitle, m_size);
    if (!m_hwnd)
    {
        log_error("Failed to create window");
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    log_debug("Initializing DirectX 11 Renderer (%ix%i)", m_size.x, m_size.y);

    if (!createDevice())
    {
        log_error("Failed to create D3D11 device");
        return false;
    }

    if (!createSwapChain(m_hwnd))
    {
        log_error("Failed to create swap chain");
        return false;
    }

    if (!createRenderTargetView())
    {
        log_error("Failed to create render target view");
        return false;
    }

    if (!createDepthStencilBuffer())
    {
        log_error("Failed to create depth stencil buffer");
        return false;
    }

    if (!createRasterizerState())
    {
        log_error("Failed to create rasterizer state");
        return false;
    }

    if (!createBlendState())
    {
        log_error("Failed to create blend state");
        return false;
    }

    setViewport();

    if (!initializeImGui())
    {
        log_warning("Failed to initialize ImGui");
    }

    m_initialized = true;
    log_debug("DirectX 11 Renderer initialized successfully");
    return true;
}

bool DX11Renderer::initializeOverlay(const char* targetWindowTitle, const char* targetWindowClass)
{
    if (m_initialized)
    {
        log_warning("Renderer already initialized");
        return true;
    }

    m_targetHwnd = findTargetWindow(targetWindowTitle, targetWindowClass);
    if (!m_targetHwnd)
    {
        log_error("Failed to find target window: %s", targetWindowTitle);
        return false;
    }

    if (!GetWindowRect(m_targetHwnd, &m_targetRect))
    {
        log_error("Failed to get target window rect");
        return false;
    }

    m_size = Vector2<LONG>(m_targetRect.right - m_targetRect.left, m_targetRect.bottom - m_targetRect.top);

    log_debug("Creating overlay window (%ix%i)", m_size.x, m_size.y);

    m_hwnd = createOverlayWindow();
    if (!m_hwnd)
    {
        log_error("Failed to create overlay window");
        return false;
    }

    if (!createDevice())
    {
        log_error("Failed to create D3D11 device");
        return false;
    }

    if (!createSwapChain(m_hwnd))
    {
        log_error("Failed to create swap chain");
        return false;
    }

    if (!createRenderTargetView())
    {
        log_error("Failed to create render target view");
        return false;
    }

    if (!createDepthStencilBuffer())
    {
        log_error("Failed to create depth stencil buffer");
        return false;
    }

    if (!createRasterizerState())
    {
        log_error("Failed to create rasterizer state");
        return false;
    }

    if (!createBlendState())
    {
        log_error("Failed to create blend state");
        return false;
    }

    setViewport();
    makeWindowTransparent(m_hwnd);

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    if (!initializeImGui())
    {
        log_warning("Failed to initialize ImGui");
    }

    m_initialized = true;
    log_debug("Overlay renderer initialized successfully");
    return true;
}

void DX11Renderer::shutdown()
{
    if (!m_initialized)
        return;

    log_debug("Shutting down DirectX 11 Renderer");

    shutdownImGui();

    if (m_context)
    {
        m_context->ClearState();
        m_context->Flush();
    }

    releaseRenderTargets();

    m_blendState.Reset();
    m_rasterizerState.Reset();
    m_depthStencilState.Reset();
    m_swapChain.Reset();
    m_dxgiFactory.Reset();
    m_context.Reset();
    m_device.Reset();

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_initialized = false;
}

bool DX11Renderer::createDevice()
{
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        &m_featureLevel,
        &m_context
    );

    if (FAILED(hr))
    {
        log_error("D3D11CreateDevice failed with error: 0x%08X", hr);
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_device.As(&dxgiDevice);
    if (FAILED(hr))
    {
        log_error("Failed to get DXGI device: 0x%08X", hr);
        return false;
    }

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr))
    {
        log_error("Failed to get DXGI adapter: 0x%08X", hr);
        return false;
    }

    hr = adapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr))
    {
        log_error("Failed to get DXGI factory: 0x%08X", hr);
        return false;
    }

    hr = m_device->CheckMultisampleQualityLevels(m_backBufferFormat, 4, &m_msaaQuality);
    if (SUCCEEDED(hr) && m_msaaQuality > 0)
    {
        m_sampleCount = 4;
        log_debug("4x MSAA supported with quality level: %i", m_msaaQuality - 1);
    }
    else
    {
        m_sampleCount = 1;
        log_debug("MSAA not supported, using no anti-aliasing");
    }

    log_debug("D3D11 device created with feature level: 0x%04X", m_featureLevel);
    return true;
}

bool DX11Renderer::createSwapChain(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_size.x;
    swapChainDesc.Height = m_size.y;
    swapChainDesc.Format = m_backBufferFormat;
    swapChainDesc.SampleDesc.Count = m_sampleCount;
    swapChainDesc.SampleDesc.Quality = m_sampleCount > 1 ? m_msaaQuality - 1 : 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
    fullscreenDesc.RefreshRate.Numerator = 60;
    fullscreenDesc.RefreshRate.Denominator = 1;
    fullscreenDesc.Windowed = TRUE;

    HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
        m_device.Get(),
        hwnd,
        &swapChainDesc,
        &fullscreenDesc,
        nullptr,
        &m_swapChain
    );

    if (FAILED(hr))
    {
        log_error("CreateSwapChainForHwnd failed with error: 0x%08X", hr);
        return false;
    }

    m_dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    log_debug("Swap chain created successfully");
    return true;
}

bool DX11Renderer::createRenderTargetView()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
    {
        log_error("Failed to get back buffer: 0x%08X", hr);
        return false;
    }

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr))
    {
        log_error("CreateRenderTargetView failed with error: 0x%08X", hr);
        return false;
    }

    log_debug("Render target view created successfully");
    return true;
}

bool DX11Renderer::createDepthStencilBuffer()
{
    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    depthBufferDesc.Width = m_size.x;
    depthBufferDesc.Height = m_size.y;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = m_sampleCount;
    depthBufferDesc.SampleDesc.Quality = m_sampleCount > 1 ? m_msaaQuality - 1 : 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    HRESULT hr = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(hr))
    {
        log_error("Failed to create depth stencil buffer: 0x%08X", hr);
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = depthBufferDesc.Format;
    depthStencilViewDesc.ViewDimension = m_sampleCount > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    hr = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilViewDesc, &m_depthStencilView);
    if (FAILED(hr))
    {
        log_error("CreateDepthStencilView failed with error: 0x%08X", hr);
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
    if (FAILED(hr))
    {
        log_error("CreateDepthStencilState failed with error: 0x%08X", hr);
        return false;
    }

    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

    log_debug("Depth stencil buffer created successfully");
    return true;
}

bool DX11Renderer::createRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.MultisampleEnable = m_sampleCount > 1;
    rasterizerDesc.AntialiasedLineEnable = FALSE;

    HRESULT hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
    if (FAILED(hr))
    {
        log_error("CreateRasterizerState failed with error: 0x%08X", hr);
        return false;
    }

    m_context->RSSetState(m_rasterizerState.Get());

    log_debug("Rasterizer state created successfully");
    return true;
}

bool DX11Renderer::createBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = m_device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr))
    {
        log_error("CreateBlendState failed with error: 0x%08X", hr);
        return false;
    }

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);

    log_debug("Blend state created successfully");
    return true;
}

void DX11Renderer::setViewport()
{
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_size.x);
    viewport.Height = static_cast<float>(m_size.y);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_context->RSSetViewports(1, &viewport);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    log_debug("Viewport set to %ix%i", m_size.x, m_size.y);
}

void DX11Renderer::releaseRenderTargets()
{
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
}

void DX11Renderer::beginFrame(float r, float g, float b, float a)
{
    if (!m_initialized)
        return;

    updateOverlayPosition();

    float clearColor[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    beginImGuiFrame();
}

void DX11Renderer::endFrame()
{
    if (!m_initialized)
        return;

    endImGuiFrame();
    renderImGui();

    UINT syncInterval = m_vsyncEnabled ? 1 : 0;
    HRESULT hr = m_swapChain->Present(syncInterval, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        log_error("Device lost during Present: 0x%08X", hr);
    }
}

void DX11Renderer::onResize(Vector2<LONG> size)
{
    if (!m_initialized || (size == m_size))
        return;

    log_debug("Resizing renderer from %ix%i to %ix%i", m_size.x, m_size.y, size.x, size.y);

    m_size = size;

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    releaseRenderTargets();

    HRESULT hr = m_swapChain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        log_warning("ResizeBuffers failed with error: 0x%08X", hr);
        return;
    }

    if (!createRenderTargetView())
    {
        log_warning("Failed to recreate render target view after resize");
        return;
    }

    if (!createDepthStencilBuffer())
    {
        log_warning("Failed to recreate depth stencil buffer after resize");
        return;
    }

    setViewport();
}

HWND DX11Renderer::createWindow(const char* title, Vector2<LONG> size)
{
    const char* className = "DX11RendererWindow";
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProcStatic;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = className;

    if (!RegisterClassExA(&wc))
    {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            log_error("Failed to register window class: %u", error);
            return nullptr;
        }
    }

    DWORD windowStyle;
    DWORD exStyle = 0;
    Vector2<LONG> windowSize;
    Vector2<LONG> windowPosition;
    
    if (m_borderlessFullscreen)
    {
        windowStyle = WS_POPUP;
        windowPosition = Vector2<LONG>(0, 0);
        windowSize = size;
    }
    else
    {
        windowStyle = WS_OVERLAPPEDWINDOW;
        RECT rect = { 0, 0, size.x, size.y };
        AdjustWindowRect(&rect, windowStyle, FALSE);
        windowPosition = Vector2<LONG>(CW_USEDEFAULT, CW_USEDEFAULT);
        windowSize = Vector2<LONG>(rect.right - rect.left, rect.bottom - rect.top);
    }

    HWND hwnd = CreateWindowExA(
        exStyle,
        className,
        title,
        windowStyle,
        windowPosition.x, windowPosition.y,
            windowSize.x,
        windowSize.y,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!hwnd)
    {
        log_error("Failed to create window: %u", GetLastError());
        return nullptr;
    }

    return hwnd;
}

HWND DX11Renderer::createOverlayWindow()
{
    const char* className = "DX11OverlayWindow";
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProcStatic;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)0;
    wc.lpszClassName = className;

    if (!RegisterClassExA(&wc))
    {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            log_error("Failed to register overlay window class: %u", error);
            return nullptr;
        }
    }

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        className,
        "Overlay",
        WS_POPUP,
        m_targetRect.left,
        m_targetRect.top,
        m_targetRect.right - m_targetRect.left,
        m_targetRect.bottom - m_targetRect.top,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!hwnd)
    {
        log_error("Failed to create overlay window: %u", GetLastError());
        return nullptr;
    }

    return hwnd;
}

void DX11Renderer::makeWindowTransparent(HWND hwnd) const
{
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    DwmExtendFrameIntoClientArea(hwnd, &m_margins);
    
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(hwnd, &bb);
    DeleteObject(bb.hRgnBlur);
}

HWND DX11Renderer::findTargetWindow(const char* windowTitle, const char* windowClass)
{
    HWND hwnd = nullptr;
    
    if (windowClass)
    {
        hwnd = FindWindowA(windowClass, windowTitle);
    }
    else
    {
        hwnd = FindWindowA(nullptr, windowTitle);
    }
    
    if (!hwnd && windowTitle)
    {
        struct FindWindowData {
            const char* title;
            HWND result;
        } data = { windowTitle, nullptr };
        
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* data = reinterpret_cast<FindWindowData*>(lParam);
            char title[256];
            if (GetWindowTextA(hwnd, title, sizeof(title)) && strstr(title, data->title))
            {
                data->result = hwnd;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&data));
        
        hwnd = data.result;
    }
    
    return hwnd;
}

void DX11Renderer::updateOverlayPosition()
{
    if (!m_targetHwnd || !m_hwnd)
        return;
    
    if (!IsWindow(m_targetHwnd))
    {
        log_warning("Target window no longer exists");
        m_shouldClose = true;
        return;
    }
    
    RECT newRect;
    if (GetWindowRect(m_targetHwnd, &newRect))
    {
        if (memcmp(&newRect, &m_targetRect, sizeof(RECT)) != 0)
        {
            m_targetRect = newRect;
            int width = newRect.right - newRect.left;
            int height = newRect.bottom - newRect.top;
            
            SetWindowPos(m_hwnd, HWND_TOPMOST, 
                newRect.left, newRect.top, width, height,
                SWP_NOACTIVATE);
            
            if (width != m_size.x || height != m_size.y)
            {
                onResize(Vector2<LONG>(width, height));
            }
        }
    }
    
    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void DX11Renderer::setOverlayInteractive(bool interactive) const
{
    if (!m_hwnd)
        return;

    LONG exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    
    if (interactive)
    {
        exStyle &= ~WS_EX_TRANSPARENT;
        SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);
        
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
            
        log_debug("Overlay set to interactive mode");
    }
    else
    {
        exStyle |= WS_EX_TRANSPARENT;
        SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);
        
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            
        log_debug("Overlay set to click-through mode");
    }
}

LRESULT CALLBACK DX11Renderer::windowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DX11Renderer* renderer = nullptr;

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        renderer = reinterpret_cast<DX11Renderer*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(renderer));
    }
    else
    {
        renderer = reinterpret_cast<DX11Renderer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (renderer)
    {
        return renderer->windowProc(hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT DX11Renderer::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_imguiInitialized)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;
    }

    switch (uMsg)
    {
    case WM_DESTROY:
        m_shouldClose = true;
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (m_initialized && wParam != SIZE_MINIMIZED)
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            onResize(Vector2<LONG>(width, height));
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            m_shouldClose = true;
            PostQuitMessage(0);
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool DX11Renderer::processMessages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_shouldClose = true;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return !m_shouldClose;
}

bool DX11Renderer::initializeImGui()
{
    if (m_imguiInitialized)
        return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    
    ImGui::StyleColorsDark();
    
    if (!ImGui_ImplWin32_Init(m_hwnd))
    {
        log_error("Failed to initialize ImGui Win32 backend");
        return false;
    }
    
    if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get()))
    {
        log_error("Failed to initialize ImGui DX11 backend");
        ImGui_ImplWin32_Shutdown();
        return false;
    }

    m_imguiInitialized = true;
    log_debug("ImGui initialized successfully");
    return true;
}

void DX11Renderer::shutdownImGui()
{
    if (!m_imguiInitialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    m_imguiInitialized = false;
    log_debug("ImGui shut down");
}

void DX11Renderer::beginImGuiFrame() const
{
    if (!m_imguiInitialized)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DX11Renderer::endImGuiFrame() const
{
    if (!m_imguiInitialized)
        return;

    ImGui::Render();
}

void DX11Renderer::renderImGui() const
{
    if (!m_imguiInitialized)
        return;

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void DX11Renderer::setBorderlessFullscreen(bool enabled)
{
    if (m_borderlessFullscreen == enabled || !m_hwnd)
        return;
        
    m_borderlessFullscreen = enabled;
    
    if (enabled)
    {
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP);
        
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, 
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                     
        if (screenWidth != m_size.x || screenHeight != m_size.y)
        {
            onResize(Vector2<LONG>(screenWidth, screenHeight));
        }
    }
    else
    {
        SetWindowLong(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        
        int windowWidth = 1280;
        int windowHeight = 720;
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2;
        
        RECT rect = { 0, 0, windowWidth, windowHeight };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        windowWidth = rect.right - rect.left;
        windowHeight = rect.bottom - rect.top;
        
        SetWindowPos(m_hwnd, HWND_TOP, x, y, windowWidth, windowHeight, 
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                     
        if (1280 != m_size.x || 720 != m_size.y)
        {
            onResize(Vector2<LONG>(1280, 720));
        }
    }
    
    log_info("Switched to %s mode", enabled ? "borderless fullscreen" : "windowed");
}
