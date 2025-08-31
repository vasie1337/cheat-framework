#include <core/core.hpp>
#include <thread>
#include <chrono>

int main() {
	Core core;
	core.with_access_adapter(AccessAdapter::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug);

	if (!core.initialize()) {
		log_critical("Failed to initialize core");
		return 1;
	}

	log_info("AssaultCube cheat initialized");

	while (core.update()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	core.shutdown();
	
	return 0;
}