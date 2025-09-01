#include <core/core.hpp>
#include <thread>
#include <chrono>

int main() {
	Core core;
	
	if (!core
		.with_access_adapter(AccessAdapterKind::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug)
		.with_window_title("AssaultCube Cheat")
		.with_target_window("AssaultCube")
		.initialize())
	{
		log_critical("Failed to initialize core");
		return 1;
	}

	while (core.update()) {
	}

	core.shutdown();
	
	return 0;
}