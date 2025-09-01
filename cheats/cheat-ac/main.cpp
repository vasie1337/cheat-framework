#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

#include <thread>
#include <chrono>

uint32_t game_base = 0x0;
uint32_t player_list = 0x0;
int player_list_count = 0;
Matrix<16> view_matrix;

void get_globals(Core *core)
{
	if (!game_base)
		game_base = static_cast<uint32_t>(core->m_access_adapter->getModule("ac_client.exe")->baseAddress);

	player_list = core->m_access_adapter->read<uint32_t>(game_base + 0x18AC04);
	player_list_count = core->m_access_adapter->read<int>(game_base + 0x18AC0C);
	view_matrix = core->m_access_adapter->read<Matrix<16>>(game_base + 0x17DFD0);
}

void get_players(Core *core)	
{
	ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

	for (int i = 1; i < player_list_count; i++)
	{
		uint32_t player_ptr = core->m_access_adapter->read<uint32_t>(player_list + i * 0x4);

		Vector3<float> position = core->m_access_adapter->read<Vector3<float>>(player_ptr + 0x4);
		Vector2<float> screen_position;
		
		if (core->WorldToScreen(position, screen_position, view_matrix))
		{
			draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 5.f, IM_COL32(255, 0, 0, 255), 8);
		}
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

	core->register_callback(get_globals);
	core->register_callback(get_players);

	while (core->update())
	{
	}

	return 0;
}
