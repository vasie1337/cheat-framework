#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <chrono>

#include <core/logger/logger.hpp>
#include <core/rendering/rendering.hpp>
#include <core/projection/projection.hpp>
#include <core/cache/update_manager.hpp>
#include <core/cache/change_detector.hpp>
#include <core/access/adapter.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

enum class TargetKind
{
    Local,
    Remote
};

class Core;

class CachedFunction
{
public:
    CachedFunction(std::function<void(Core&)> func, int interval_ms, Core& core)
        : m_func(std::move(func)), m_interval_ms(interval_ms), m_last_call_time(0), m_core(core)
    {
    }

    void operator()()
    {
        if (should_execute())
        {
            m_func(m_core);
            m_last_call_time = get_current_time();
        }
    }

    bool should_execute() const
    {
        if (m_interval_ms == -1)
            return true;
        return (get_current_time() - m_last_call_time) >= static_cast<uint64_t>(m_interval_ms);
    }

private:
    std::function<void(Core&)> m_func;
    int m_interval_ms;
    mutable uint64_t m_last_call_time;
    Core& m_core;

    uint64_t get_current_time() const
    {
        using namespace std::chrono;
        return duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

class Core
{
public:
    Core() = default;
    ~Core() = default;

    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    Core(Core&&) = default;
    Core& operator=(Core&&) = default;

    bool initialize();
    bool update();

    Core& with_target_type(TargetKind target_type)
    {
        m_target_type = target_type;
        return *this;
    }

    Core& with_logger_backend(LoggerBackend backend)
    {
        m_logger_backend_kind = backend;
        return *this;
    }

    Core& with_logger_level(LogLevel level)
    {
        m_logger_level_kind = level;
        return *this;
    }

    Core& with_window_title(std::string title)
    {
        m_window_title = std::move(title);
        return *this;
    }

    Core& with_target(std::string window_title, std::string process_name)
    {
        m_target_window_title = std::move(window_title);
        m_target_process_name = std::move(process_name);
        return *this;
    }

    Core& with_widgets_enabled(bool enabled = true)
    {
        m_show_widgets = enabled;
        return *this;
    }

    void register_function(std::function<void(Core&)> function, int interval_ms = -1)
    {
        m_functions.emplace_back(
            std::make_unique<CachedFunction>(
                std::move(function), interval_ms, *this
            )
        );
    }

    std::unique_ptr<AccessAdapter> m_access_adapter;
    std::unique_ptr<DX11Renderer> m_renderer;
    std::unique_ptr<ProjectionUtils> m_projection_utils;
    std::unique_ptr<UpdateManager> m_update_manager;

private:
    TargetKind m_target_type = TargetKind::Local;
    LoggerBackend m_logger_backend_kind = LoggerBackend::Console;
    LogLevel m_logger_level_kind = LogLevel::Debug;
    bool m_show_widgets = false;

    std::string m_window_title;
    std::string m_target_window_title;
    std::string m_target_process_name;

    std::vector<std::unique_ptr<CachedFunction>> m_functions;

    void execute_all_functions()
    {
        for (auto& func : m_functions)
        {
            (*func)();
        }
    }
};