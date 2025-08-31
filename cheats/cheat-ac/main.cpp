#include <core/core.hpp>
#include <thread>
#include <chrono>
#include <Windows.h>

int main() {
	Core core;
	core.with_access_adapter(AccessAdapterKind::Local)
		.with_logger_backend(LoggerBackend::Both)  // Log to both console and file
		.with_logger_level(LogLevel::Debug)
		.with_window_title("AssaultCube Cheat")
		.with_window_size(800, 600);

	if (!core.initialize()) {
		log_critical("Failed to initialize core");
		return 1;
	}

	log_info("AssaultCube cheat initialized");
	log_info("Press ESC in the window to exit...");

	// Main loop
	while (core.update()) {
		// In a real cheat, you would:
		// 1. Read game memory
		// 2. Update ESP data
		// 3. Render overlay
		
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	core.shutdown();
	
	return 0;
}