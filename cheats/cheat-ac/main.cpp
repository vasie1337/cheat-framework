#include <core/core.hpp>
#include <thread>
#include <chrono>

void get_globals(AccessAdapter *access_adapter)
{
	auto module = access_adapter->getModule("ac_client.exe");
	if (!module) {
		log_error("Failed to get ac_client.exe module");
		return;
	}
	uintptr_t base_address = module->baseAddress;
	uintptr_t client_state = access_adapter->read<uintptr_t>(base_address + 0x13475C0);
}

void get_players(AccessAdapter *access_adapter)
{
	uintptr_t entity_list = access_adapter->read<uintptr_t>(0 + 0x1379D8);
	uintptr_t num_players = access_adapter->read<uintptr_t>(entity_list + 0x1379E0);

	for (int i = 0; i < num_players; i++)
	{
		uintptr_t player = access_adapter->read<uintptr_t>(entity_list + 0x1379E8 + i * 0x1379F0);
		std::string player_name = access_adapter->read<std::string>(player + 0x1379F8);
	}
}

int main()
{
	std::unique_ptr<Core> core = std::make_unique<Core>();

	core->with_target_type(TargetKind::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug)
		.with_target("AssaultCube", nullptr, "ac_client.exe")
		.with_window_title("AssaultCube Cheat");

	if (!core->initialize())
	{
		log_critical("Failed to initialize core");
		return 1;
	}

	// core->register_callback(get_globals);
	// core->register_callback(get_players);

	while (core->update())
	{
	}

	return 0;
}