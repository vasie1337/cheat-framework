#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

#include <core/logger/logger.hpp>
#include <core/rendering/rendering.hpp>
#include <core/access/adapter.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

enum class TargetKind
{
	Local,
	Remote
};

class Core
{
public:
	Core() = default;
	~Core() = default;

	bool initialize();

	Core &with_target_type(TargetKind target_type)
	{
		m_target_type = target_type;
		return *this;
	}
	Core &with_logger_backend(LoggerBackend backend)
	{
		m_logger_backend_kind = backend;
		return *this;
	}
	Core &with_logger_level(LogLevel level)
	{
		m_logger_level_kind = level;
		return *this;
	}
	Core &with_window_title(const char *title)
	{
		m_window_title = title;
		return *this;
	}
	Core &with_target(const char *title, const char *className, const char *processName)
	{
		m_target_window_title = title;
		m_target_window_class = className;
		m_target_process_name = processName;
		return *this;
	}

	void register_callback(std::function<void(Core*)> callback)
	{
		m_callbacks.emplace_back(callback);
	}

	bool update();

	std::unique_ptr<AccessAdapter> m_access_adapter;
	std::unique_ptr<DX11Renderer> m_renderer;

	// Cheat functions:
	bool WorldToScreen(const Vector3<float>& VecOrigin, Vector2<float>& VecScreen, const Matrix<16>& Matrix) const;

private:
	TargetKind m_target_type = TargetKind::Local;
	LoggerBackend m_logger_backend_kind = LoggerBackend::Console;
	LogLevel m_logger_level_kind = LogLevel::Debug;

	std::vector<std::function<void(Core*)>> m_callbacks;

	const char *m_window_title = nullptr;
	const char *m_target_window_title = nullptr;
	const char *m_target_window_class = nullptr;
	const char *m_target_process_name = nullptr;

	bool m_show_widgets = false;
	bool m_did_shutdown = false;
};