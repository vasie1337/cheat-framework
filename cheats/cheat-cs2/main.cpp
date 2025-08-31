#include <core/core.hpp>
#include <thread>
#include <chrono>

int main() {
	Core core;
	core.with_access_adapter(AccessAdapterKind::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug)
		.with_window_title("CS2 Cheat Framework")
		.with_window_size(1280, 720);

	if (!core.initialize()) {
		log_critical("Failed to initialize core");
		return 1;
	}

	log_info("Counter-Strike 2 cheat initialized");

	while (core.update()) {
	}

	core.shutdown();
	
	return 0;
}