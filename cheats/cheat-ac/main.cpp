#include <core/core.hpp>
#include <thread>
#include <chrono>

int main() {
	Core core;
	core.with_name("AssaultCube")
		.with_access_adapter(AccessAdapter::Local)
		.with_render_backend(RenderBackend::DX11)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug);

	if (!core.initialize()) {
		fprintf(stderr, "Failed to initialize core\n");
		return 1;
	}

	while (core.update()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	core.shutdown();
	
	return 0;
}