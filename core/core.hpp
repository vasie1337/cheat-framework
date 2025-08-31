#pragma once

#include <string>
#include <memory>
#include <core/logger/logger.hpp>
#include <core/rendering/rendering.hpp>

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
	Core& with_window_size(int width, int height) { m_window_width = width; m_window_height = height; return *this; }

	bool update();
	void shutdown();

	DX11Renderer* getRenderer() const { return m_renderer.get(); }

private:
	AccessAdapterKind m_access_adapter_kind = AccessAdapterKind::Local;
	LoggerBackend m_logger_backend_kind = LoggerBackend::Console;
	LogLevel m_logger_level_kind = LogLevel::Debug;
	
	std::unique_ptr<DX11Renderer> m_renderer;
	
	const char* m_window_title = "Cheat Framework";
	int m_window_width = 1280;
	int m_window_height = 720;
};	