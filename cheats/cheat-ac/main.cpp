#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>

#include <thread>
#include <chrono>

struct matrix { float matrix[16]; };

uint32_t game_base = 0x0;
uint32_t player_list = 0x0;
int player_list_count = 0;
matrix view_matrix;

bool WorldToScreen(Core *core, const Vector3<float>& VecOrigin, Vector2<float>& VecScreen, float* Matrix) {
	Vector2<float> clipCoords;
	clipCoords.x = VecOrigin.x * Matrix[0] + VecOrigin.y * Matrix[4] + VecOrigin.z * Matrix[8] + Matrix[12];
	clipCoords.y = VecOrigin.x * Matrix[1] + VecOrigin.y * Matrix[5] + VecOrigin.z * Matrix[9] + Matrix[13];
	float W = VecOrigin.x * Matrix[3] + VecOrigin.y * Matrix[7] + VecOrigin.z * Matrix[11] + Matrix[15];

	if (W < 0.01f)
		return false;

	Vector2<float> NDC;
	NDC.x = clipCoords.x / W;
	NDC.y = clipCoords.y / W;


	VecScreen.x = (NDC.x + 1.0f) * core->m_renderer->getSize().x * 0.5f;
	VecScreen.y = (1.0f - NDC.y) * core->m_renderer->getSize().y * 0.5f;
	return true;
}

void get_globals(Core *core)
{
	log_debug("get_globals");
	if (!game_base)
		game_base = core->m_access_adapter->getModule("ac_client.exe")->baseAddress;

	player_list = core->m_access_adapter->read<uint32_t>(game_base + 0x18AC04);
	player_list_count = core->m_access_adapter->read<int>(game_base + 0x18AC0C);
	view_matrix = core->m_access_adapter->read<matrix>(game_base + 0x17DFD0);
	log_debug("game_base: 0x%llX", game_base);
}

void get_players(Core *core)	
{
	ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
	log_debug("get_players");

	for (int i = 1; i < player_list_count; i++)
	{
		uint32_t player_ptr = core->m_access_adapter->read<uint32_t>(player_list + i * 0x4);
		// log_debug("player_ptr: 0x%llX", player_ptr);

		Vector3<float> position = core->m_access_adapter->read<Vector3<float>>(player_ptr + 0x4);
		Vector2<float> screen_position;
		if (WorldToScreen(core, position, screen_position, view_matrix.matrix))
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
