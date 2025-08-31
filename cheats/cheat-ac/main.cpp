#include <core/core.hpp>
#include <thread>
#include <chrono>

int main() {
	Core core;
	core.with_access_adapter(AccessAdapterKind::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug)
		.with_window_title("AssaultCube Cheat")
		.with_window_size(800, 600);

	if (!core.initialize()) {
		log_critical("Failed to initialize core");
		return 1;
	}

	while (core.update()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	core.shutdown();
	
	return 0;
}