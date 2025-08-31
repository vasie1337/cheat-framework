#include <core/core.hpp>
#include <thread>
#include <chrono>

int main() {
	Core core;
	core.with_access_adapter(AccessAdapterKind::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug)
		.with_window_title("CS2 Cheat")
		.with_target_window("Counter-Strike 2");

	if (!core.initialize()) {
		log_critical("Failed to initialize core");
		return 1;
	}

	while (core.update()) {
	}

	core.shutdown();

	return 0;
}