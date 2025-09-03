#include "rendering.hpp"
#include <core/logger/logger.hpp>
#include <stdexcept>
#include <cstring>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

DX11Renderer::DX11Renderer()
    : m_hwnd(nullptr), m_size(0, 0), m_initialized(false), m_should_close(false), m_feature_level(D3D_FEATURE_LEVEL_11_0), m_back_buffer_format(DXGI_FORMAT_R8G8B8A8_UNORM), m_msaa_quality(0), m_sample_count(1), m_target_hwnd(nullptr), m_imgui_initialized(false)
{
    m_target_rect = {0, 0, 0, 0};
    m_margins = {-1, -1, -1, -1};
}

DX11Renderer::~DX11Renderer()
{
    shutdown();
}

bool DX11Renderer::initialize(std::string window_title, std::string target_window_title)
{
    if (m_initialized)
    {
        log_warning("Renderer already initialized");
        return true;
    }

    bool is_overlay = !target_window_title.empty();
    
    if (is_overlay)
    {
        m_target_hwnd = find_target_window(target_window_title);
        if (!m_target_hwnd)
        {
            log_error("Failed to find target window: %s", target_window_title.c_str());
            return false;
        }

        if (!GetWindowRect(m_target_hwnd, &m_target_rect))
        {
            log_error("Failed to get target window rect");
            return false;
        }

        m_size = vec2_t<LONG>(m_target_rect.right - m_target_rect.left, m_target_rect.bottom - m_target_rect.top);

        log_debug("Creating overlay window (%ix%i)", m_size.x, m_size.y);

        m_hwnd = create_overlay_window();
        if (!m_hwnd)
        {
            log_error("Failed to create overlay window");
            return false;
        }
    }
    else
    {
        m_size = vec2_t<LONG>(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

        m_hwnd = create_window(window_title, m_size);
        if (!m_hwnd)
        {
            log_error("Failed to create window");
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        log_debug("Initializing DirectX 11 Renderer (%ix%i)", m_size.x, m_size.y);
    }

    if (!create_device())
    {
        log_error("Failed to create D3D11 device");
        return false;
    }

    if (!create_swap_chain(m_hwnd))
    {
        log_error("Failed to create swap chain");
        return false;
    }

    if (!create_render_target_view())
    {
        log_error("Failed to create render target view");
        return false;
    }

    if (!create_depth_stencil_buffer())
    {
        log_error("Failed to create depth stencil buffer");
        return false;
    }

    if (!create_rasterizer_state())
    {
        log_error("Failed to create rasterizer state");
        return false;
    }

    if (!create_blend_state())
    {
        log_error("Failed to create blend state");
        return false;
    }

    set_viewport();

    if (is_overlay)
    {
        make_window_transparent(m_hwnd);
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }

    if (!initialize_imgui())
    {
        log_warning("Failed to initialize ImGui");
    }

    m_initialized = true;
    log_debug("%s renderer initialized successfully", is_overlay ? "Overlay" : "DirectX 11");
    return true;
}

void DX11Renderer::shutdown()
{
    if (!m_initialized)
        return;

    log_debug("Shutting down DirectX 11 Renderer");

    shutdown_imGui();

    if (m_context)
    {
        m_context->ClearState();
        m_context->Flush();
    }

    release_render_targets();

    m_blend_state.Reset();
    m_rasterizer_state.Reset();
    m_depth_stencil_state.Reset();
    m_swap_chain.Reset();
    m_dxgi_factory.Reset();
    m_context.Reset();
    m_device.Reset();

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_initialized = false;
}

bool DX11Renderer::create_device()
{
    UINT create_device_flags = 0;
#ifdef _DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] =
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
        create_device_flags,
        feature_levels,
        ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION,
        &m_device,
        &m_feature_level,
        &m_context);

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

    hr = adapter->GetParent(IID_PPV_ARGS(&m_dxgi_factory));
    if (FAILED(hr))
    {
        log_error("Failed to get DXGI factory: 0x%08X", hr);
        return false;
    }

    hr = m_device->CheckMultisampleQualityLevels(m_back_buffer_format, 4, &m_msaa_quality);
    if (SUCCEEDED(hr) && m_msaa_quality > 0)
    {
        m_sample_count = 4;
        log_debug("4x MSAA supported with quality level: %i", m_msaa_quality - 1);
    }
    else
    {
        m_sample_count = 1;
        log_debug("MSAA not supported, using no anti-aliasing");
    }

    log_debug("D3D11 device created with feature level: 0x%04X", m_feature_level);
    return true;
}

bool DX11Renderer::create_swap_chain(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_size.x;
    swapChainDesc.Height = m_size.y;
    swapChainDesc.Format = m_back_buffer_format;
    swapChainDesc.SampleDesc.Count = m_sample_count;
    swapChainDesc.SampleDesc.Quality = m_sample_count > 1 ? m_msaa_quality - 1 : 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
    fullscreenDesc.RefreshRate.Numerator = 60;
    fullscreenDesc.RefreshRate.Denominator = 1;
    fullscreenDesc.Windowed = TRUE;

    HRESULT hr = m_dxgi_factory->CreateSwapChainForHwnd(
        m_device.Get(),
        hwnd,
        &swapChainDesc,
        &fullscreenDesc,
        nullptr,
        &m_swap_chain);

    if (FAILED(hr))
    {
        log_error("CreateSwapChainForHwnd failed with error: 0x%08X", hr);
        return false;
    }

    m_dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    log_debug("Swap chain created successfully");
    return true;
}

bool DX11Renderer::create_render_target_view()
{
    ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (FAILED(hr))
    {
        log_error("Failed to get back buffer: 0x%08X", hr);
        return false;
    }

    hr = m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_render_target_view);
    if (FAILED(hr))
    {
        log_error("CreateRenderTargetView failed with error: 0x%08X", hr);
        return false;
    }

    log_debug("Render target view created successfully");
    return true;
}

bool DX11Renderer::create_depth_stencil_buffer()
{
    D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
    depth_buffer_desc.Width = m_size.x;
    depth_buffer_desc.Height = m_size.y;
    depth_buffer_desc.MipLevels = 1;
    depth_buffer_desc.ArraySize = 1;
    depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_buffer_desc.SampleDesc.Count = m_sample_count;
    depth_buffer_desc.SampleDesc.Quality = m_sample_count > 1 ? m_msaa_quality - 1 : 0;
    depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_buffer_desc.CPUAccessFlags = 0;
    depth_buffer_desc.MiscFlags = 0;

    HRESULT hr = m_device->CreateTexture2D(&depth_buffer_desc, nullptr, &m_depth_stencil_buffer);
    if (FAILED(hr))
    {
        log_error("Failed to create depth stencil buffer: 0x%08X", hr);
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
    depth_stencil_view_desc.Format = depth_buffer_desc.Format;
    depth_stencil_view_desc.ViewDimension = m_sample_count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    depth_stencil_view_desc.Texture2D.MipSlice = 0;

    hr = m_device->CreateDepthStencilView(m_depth_stencil_buffer.Get(), &depth_stencil_view_desc, &m_depth_stencil_view);
    if (FAILED(hr))
    {
        log_error("CreateDepthStencilView failed with error: 0x%08X", hr);
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    depth_stencil_desc.DepthEnable = TRUE;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
    depth_stencil_desc.StencilEnable = FALSE;

    hr = m_device->CreateDepthStencilState(&depth_stencil_desc, &m_depth_stencil_state);
    if (FAILED(hr))
    {
        log_error("CreateDepthStencilState failed with error: 0x%08X", hr);
        return false;
    }

    m_context->OMSetDepthStencilState(m_depth_stencil_state.Get(), 0);

    log_debug("Depth stencil buffer created successfully");
    return true;
}

bool DX11Renderer::create_rasterizer_state()
{
    D3D11_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_BACK;
    rasterizer_desc.FrontCounterClockwise = FALSE;
    rasterizer_desc.DepthBias = 0;
    rasterizer_desc.DepthBiasClamp = 0.0f;
    rasterizer_desc.SlopeScaledDepthBias = 0.0f;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ScissorEnable = FALSE;
    rasterizer_desc.MultisampleEnable = m_sample_count > 1;
    rasterizer_desc.AntialiasedLineEnable = FALSE;

    HRESULT hr = m_device->CreateRasterizerState(&rasterizer_desc, &m_rasterizer_state);
    if (FAILED(hr))
    {
        log_error("CreateRasterizerState failed with error: 0x%08X", hr);
        return false;
    }

    m_context->RSSetState(m_rasterizer_state.Get());

    log_debug("Rasterizer state created successfully");
    return true;
}

bool DX11Renderer::create_blend_state()
{
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = m_device->CreateBlendState(&blend_desc, &m_blend_state);
    if (FAILED(hr))
    {
        log_error("CreateBlendState failed with error: 0x%08X", hr);
        return false;
    }

    float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    m_context->OMSetBlendState(m_blend_state.Get(), blend_factor, 0xffffffff);

    log_debug("Blend state created successfully");
    return true;
}

void DX11Renderer::set_viewport()
{
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_size.x);
    viewport.Height = static_cast<float>(m_size.y);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_context->RSSetViewports(1, &viewport);
    m_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), m_depth_stencil_view.Get());

    log_debug("Viewport set to %ix%i", m_size.x, m_size.y);
}

void DX11Renderer::release_render_targets()
{
    m_render_target_view.Reset();
    m_depth_stencil_view.Reset();
    m_depth_stencil_buffer.Reset();
}

void DX11Renderer::begin_frame(float r, float g, float b, float a)
{
    if (!m_initialized)
        return;

    update_overlay_position();

    float clear_color[4] = {r, g, b, a};
    m_context->ClearRenderTargetView(m_render_target_view.Get(), clear_color);
    m_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    begin_imgui_frame();
}

void DX11Renderer::end_frame()
{
    if (!m_initialized)
        return;

    end_imgui_frame();
    render_imgui();

    HRESULT hr = m_swap_chain->Present(0, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        log_error("Device lost during Present: 0x%08X", hr);
    }
}

void DX11Renderer::on_resize(vec2_t<LONG> size)
{
    if (!m_initialized || (size == m_size))
        return;

    log_debug("Resizing renderer from %ix%i to %ix%i", m_size.x, m_size.y, size.x, size.y);

    m_size = size;

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    release_render_targets();

    HRESULT hr = m_swap_chain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        log_warning("ResizeBuffers failed with error: 0x%08X", hr);
        return;
    }

    if (!create_render_target_view())
    {
        log_warning("Failed to recreate render target view after resize");
        return;
    }

    if (!create_depth_stencil_buffer())
    {
        log_warning("Failed to recreate depth stencil buffer after resize");
        return;
    }

    set_viewport();
}

HWND DX11Renderer::create_window(std::string title, vec2_t<LONG> size)
{
    const char *class_name = "DX11RendererWindow";
    HINSTANCE h_instance = GetModuleHandle(nullptr);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc_static;
    wc.hInstance = h_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = class_name;

    if (!RegisterClassExA(&wc))
    {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            log_error("Failed to register window class: %u", error);
            return nullptr;
        }
    }

    HWND hwnd = CreateWindowExA(
        0,
        class_name,
        title.c_str(),
        WS_POPUP,
        0, 0,
        size.x,
        size.y,
        nullptr,
        nullptr,
        h_instance,
        this);

    if (!hwnd)
    {
        log_error("Failed to create window: %u", GetLastError());
        return nullptr;
    }

    return hwnd;
}

HWND DX11Renderer::create_overlay_window()
{
    const char *class_name = "DX11OverlayWindow";
    HINSTANCE h_instance = GetModuleHandle(nullptr);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc_static;
    wc.hInstance = h_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)0;
    wc.lpszClassName = class_name;

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
        class_name,
        "Overlay",
        WS_POPUP,
        m_target_rect.left,
        m_target_rect.top,
        m_target_rect.right - m_target_rect.left,
        m_target_rect.bottom - m_target_rect.top,
        nullptr,
        nullptr,
        h_instance,
        this);

    if (!hwnd)
    {
        log_error("Failed to create overlay window: %u", GetLastError());
        return nullptr;
    }

    return hwnd;
}

void DX11Renderer::make_window_transparent(HWND hwnd) const
{
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    DwmExtendFrameIntoClientArea(hwnd, &m_margins);

    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(hwnd, &bb);
    DeleteObject(bb.hRgnBlur);
}

HWND DX11Renderer::find_target_window(std::string window_title)
{
    return FindWindowA(nullptr, window_title.c_str());
}

void DX11Renderer::update_overlay_position()
{
    if (!m_target_hwnd || !m_hwnd)
        return;

    if (!IsWindow(m_target_hwnd))
    {
        log_warning("Target window no longer exists");
        m_should_close = true;
        return;
    }

    RECT new_rect;
    if (GetWindowRect(m_target_hwnd, &new_rect))
    {
        if (memcmp(&new_rect, &m_target_rect, sizeof(RECT)) != 0)
        {
            m_target_rect = new_rect;
            LONG width = new_rect.right - new_rect.left;
            LONG height = new_rect.bottom - new_rect.top;

            SetWindowPos(m_hwnd, HWND_TOPMOST,
                         new_rect.left, new_rect.top, width, height,
                         SWP_NOACTIVATE);

            if (width != m_size.x || height != m_size.y)
            {
                on_resize(vec2_t<LONG>(width, height));
            }
        }
    }

    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void DX11Renderer::set_overlay_interactive(bool interactive) const
{
    if (!m_hwnd)
        return;

    LONG ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);

    if (interactive)
    {
        ex_style &= ~WS_EX_TRANSPARENT;
        SetWindowLong(m_hwnd, GWL_EXSTYLE, ex_style);

        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

		SetForegroundWindow(m_hwnd);

        log_debug("Overlay set to interactive mode");
    }
    else
    {
        ex_style |= WS_EX_TRANSPARENT;
        SetWindowLong(m_hwnd, GWL_EXSTYLE, ex_style);

        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        SetForegroundWindow(m_target_hwnd);

        log_debug("Overlay set to click-through mode");
    }
}

LRESULT CALLBACK DX11Renderer::window_proc_static(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DX11Renderer *renderer = nullptr;

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT *p_create = reinterpret_cast<CREATESTRUCT *>(lParam);
        renderer = reinterpret_cast<DX11Renderer *>(p_create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(renderer));
    }
    else
    {
        renderer = reinterpret_cast<DX11Renderer *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (renderer)
    {
        return renderer->window_proc(hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT DX11Renderer::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_imgui_initialized)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;
    }

    switch (uMsg)
    {
    case WM_DESTROY:
        m_should_close = true;
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (m_initialized && wParam != SIZE_MINIMIZED)
        {
            LONG width = LOWORD(lParam);
            LONG height = HIWORD(lParam);
            on_resize(vec2_t<LONG>(width, height));
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            m_should_close = true;
            PostQuitMessage(0);
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool DX11Renderer::process_messages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_should_close = true;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return !m_should_close;
}

bool DX11Renderer::initialize_imgui()
{
    if (m_imgui_initialized)
        return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
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

    m_imgui_initialized = true;
    log_debug("ImGui initialized successfully");
    return true;
}

void DX11Renderer::shutdown_imGui()
{
    if (!m_imgui_initialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_imgui_initialized = false;
    log_debug("ImGui shut down");
}

void DX11Renderer::begin_imgui_frame() const
{
    if (!m_imgui_initialized)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DX11Renderer::end_imgui_frame() const
{
    if (!m_imgui_initialized)
        return;

    ImGui::Render();
}

void DX11Renderer::render_imgui() const
{
    if (!m_imgui_initialized)
        return;

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
