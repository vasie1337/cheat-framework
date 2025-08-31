#pragma once

#include <string>
#include <core/logger.hpp>

enum class AccessAdapter {
	Local,
	Remote
};

enum class RenderBackend {
	DX11,
};

class Core {
public:
	bool initialize();

	Core& with_access_adapter(AccessAdapter adapter) {
		m_access_adapter = adapter;
		return *this;	
	}
	Core& with_render_backend(RenderBackend backend) {
		m_render_backend = backend;
		return *this;
	}
	Core& with_name(const std::string& name) {
		m_cheat_name = name;
		return *this;
	}
	Core& with_logger_backend(LoggerBackend backend) {
		m_logger_backend = backend;
		return *this;
	}
	Core& with_logger_level(LogLevel level) {
		m_logger_level = level;
		return *this;
	}

	bool update();
	void shutdown();

private:
	AccessAdapter m_access_adapter;
	RenderBackend m_render_backend;
	LoggerBackend m_logger_backend = LoggerBackend::Console;
	LogLevel m_logger_level = LogLevel::Debug;
	std::string m_cheat_name;
};	