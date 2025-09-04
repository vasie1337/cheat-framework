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

    bool initialize(std::string window_title, std::string target_window_title = "");

    void shutdown();

    bool initialize_imgui();
    void shutdown_imGui();

    void begin_imgui_frame() const;
    void end_imgui_frame() const;
    void render_imgui() const;

    void begin_frame(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void end_frame();

    void on_resize(vec2_t<LONG> size);
    bool process_messages();

    HWND get_window() const { return m_hwnd; }
    ID3D11Device *get_device() const { return m_device.Get(); }
    ID3D11DeviceContext *get_context() const { return m_context.Get(); }
    ID3D11RenderTargetView *get_render_target_view() const { return m_render_target_view.Get(); }
    ID3D11DepthStencilView *get_depth_stencil_view() const { return m_depth_stencil_view.Get(); }

    vec2_t<LONG> get_size() const { return m_size; }
    float get_fps() const;

    bool is_initialized() const { return m_initialized; }

    void update_overlay_position();
    void set_overlay_interactive(bool interactive) const;
    HWND get_target_window() const { return m_target_hwnd; }

private:
    DXGI_RATIONAL get_refresh_rate() const;

    bool create_device();
    bool create_swap_chain(HWND hwnd);
    bool create_render_target_view();
    bool create_depth_stencil_buffer();
    bool create_rasterizer_state();
    bool create_blend_state();
    void set_viewport();

    void release_render_targets();

    HWND create_window(std::string title, vec2_t<LONG> size);
    HWND create_overlay_window();
    void make_window_transparent(HWND hwnd) const;
    HWND find_target_window(std::string window_title);
    static LRESULT CALLBACK window_proc_static(HWND hwnd, UINT uMsg, WPARAM w_param, LPARAM l_param);
    LRESULT window_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain1> m_swap_chain;
    ComPtr<IDXGIFactory2> m_dxgi_factory;

    ComPtr<ID3D11RenderTargetView> m_render_target_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11DepthStencilState> m_depth_stencil_state;

    ComPtr<ID3D11RasterizerState> m_rasterizer_state;
    ComPtr<ID3D11BlendState> m_blend_state;

    HWND m_hwnd;
    vec2_t<LONG> m_size;
    float m_fps;
    bool m_initialized;
    bool m_imgui_initialized;
    bool m_should_close;

    D3D_FEATURE_LEVEL m_feature_level;
    DXGI_FORMAT m_back_buffer_format;
    UINT m_msaa_quality;
    UINT m_sample_count;

    HWND m_target_hwnd;
    RECT m_target_rect;
    MARGINS m_margins;
    DWORD m_last_position_check;
    RECT m_smoothed_rect;
};
