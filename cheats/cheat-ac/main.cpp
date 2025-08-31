#include <core/core.hpp>

#include <iostream>

int main() {
	Core core;
	core.with_name("AssaultCube")
		.with_access_adapter(AccessAdapter::Local)
		.with_render_backend(RenderBackend::DX11);

	if (!core.initialize()) {
		std::cerr << "Failed to initialize core" << std::endl;
		return 1;
	}

	core.shutdown();
	return 0;
}