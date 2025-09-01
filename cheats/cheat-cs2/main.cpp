#include <core/core.hpp>
#include <thread>
#include <chrono>

int main()
{
	std::unique_ptr<Core> core = std::make_unique<Core>();

	if (!core->with_access_adapter(AccessAdapterKind::Local)
			 ->with_logger_backend(LoggerBackend::Console)
			 ->with_logger_level(LogLevel::Debug)
			 ->with_window_title("CS2 Cheat")
			 ->with_target("Counter-Strike 2", nullptr, "cs2.exe")
			 ->initialize())
	{
		log_critical("Failed to initialize core");
		return 1;
	}

	while (core->update())
	{
	}

	core->shutdown();

	return 0;
}