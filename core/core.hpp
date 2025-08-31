#pragma once

#include <string>

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

	void update();
	void shutdown();

private:
	AccessAdapter m_access_adapter;
	RenderBackend m_render_backend;
	std::string m_cheat_name;
};	