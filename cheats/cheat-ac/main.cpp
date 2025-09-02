#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

#include <thread>
#include <chrono>

uint32_t game_base = 0x0;
uint32_t player_list_ptr = 0x0;
int player_list_count = 0;
Matrix<16> view_matrix;

void get_globals(Core* core)
{
	if (!game_base)
		game_base = static_cast<uint32_t>(core->m_access_adapter->getModule("ac_client.exe")->baseAddress);

	auto scatter_handle = core->m_access_adapter->createScatterHandle();

	core->m_access_adapter->addScatterRead(scatter_handle, game_base + 0x18AC04, &player_list_ptr, sizeof(uint32_t));
	core->m_access_adapter->addScatterRead(scatter_handle, game_base + 0x18AC0C, &player_list_count, sizeof(int));
	core->m_access_adapter->addScatterRead(scatter_handle, game_base + 0x17DFD0, &view_matrix, sizeof(Matrix<16>));

	core->m_access_adapter->executeScatterRead(scatter_handle);
	core->m_access_adapter->destroyScatterHandle(scatter_handle);
}

void get_players(Core* core)
{
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	if (player_list_count <= 1)
	{
		return;
	}

	auto scatter_handle = core->m_access_adapter->createScatterHandle();

	std::vector<uint32_t> player_object_pointers;
	player_object_pointers.resize(player_list_count);

	for (int i = 0; i < player_list_count; ++i)
	{
		core->m_access_adapter->addScatterRead(
			scatter_handle,
			player_list_ptr + i * sizeof(uint32_t),
			&player_object_pointers[i],
			sizeof(uint32_t)
		);
	}

	core->m_access_adapter->executeScatterRead(scatter_handle);

	std::vector<Vector3<float>> player_positions;
	player_positions.resize(player_list_count);

	for (int i = 0; i < player_list_count; ++i)
	{
		if (player_object_pointers[i] != 0)
		{
			core->m_access_adapter->addScatterRead(
				scatter_handle,
				player_object_pointers[i] + 0x4,
				&player_positions[i],
				sizeof(Vector3<float>)
			);
		}
	}

	core->m_access_adapter->executeScatterRead(scatter_handle);
	core->m_access_adapter->destroyScatterHandle(scatter_handle);

	for (int i = 1; i < player_list_count; ++i)
	{
		Vector2<float> screen_position;
		if (player_object_pointers[i] != 0 &&
			core->WorldToScreen(player_positions[i], screen_position, view_matrix))
		{
			draw_list->AddCircleFilled(
				ImVec2(screen_position.x, screen_position.y),
				5.f,
				IM_COL32(255, 0, 0, 255),
				8
			);
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
