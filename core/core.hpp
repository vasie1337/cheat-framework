#pragma once

#include <string>
#include <memory>
#include <core/logger/logger.hpp>
#include <core/rendering/rendering.hpp>
#include <core/access/adapter.hpp>

enum class AccessAdapterKind {
	Local,
	Remote
};

class Core {
public:
	Core();
	~Core();

	bool initialize();

	Core& with_access_adapter(AccessAdapterKind adapter) { m_access_adapter_kind = adapter; return *this; }
	Core& with_logger_backend(LoggerBackend backend) { m_logger_backend_kind = backend; return *this; }
	Core& with_logger_level(LogLevel level) { m_logger_level_kind = level; return *this; }
	Core& with_window_title(const char* title) { m_window_title = title; return *this; }
	Core& with_target(const char* title, const char* className = nullptr, const char* processName = nullptr) { 
		m_target_window_title = title; 
		m_target_window_class = className; 
		m_target_process_name = processName;
		return *this; 
	}

	bool update();
	void shutdown();

private:
	AccessAdapterKind m_access_adapter_kind = AccessAdapterKind::Local;
	LoggerBackend m_logger_backend_kind = LoggerBackend::Console;
	LogLevel m_logger_level_kind = LogLevel::Debug;
	
	std::unique_ptr<DX11Renderer> m_renderer;
	std::unique_ptr<AccessAdapter> m_access_adapter;
	
	const char* m_window_title = nullptr;
	const char* m_target_window_title = nullptr;
	const char* m_target_window_class = nullptr;
	const char* m_target_process_name = nullptr;

	bool m_show_widgets = false;
};	