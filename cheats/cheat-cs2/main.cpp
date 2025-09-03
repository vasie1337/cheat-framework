#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

#include <thread>
#include <chrono>

uintptr_t client_dll = 0x0;

uintptr_t ent_list = 0x0;
uintptr_t local_player_ptr = 0x0;
Matrix4x4<float> view_matrix;

void get_globals(Core* core)
{
	if (!client_dll)
		client_dll = core->m_access_adapter->getModule("client.dll")->baseAddress;

	core->m_access_adapter->addScatterRead(client_dll + 0x1BEC440, &local_player_ptr, sizeof(local_player_ptr));
	core->m_access_adapter->addScatterRead(client_dll + 0x1D0FE08, &ent_list, sizeof(ent_list));
	core->m_access_adapter->addScatterRead(client_dll + 0x1E2D030, &view_matrix, sizeof(Matrix4x4<float>));

	core->m_access_adapter->executeScatterRead();
}

class Entity
{
public:
	uintptr_t instance;
	uintptr_t identity;
	uintptr_t game_scene_node;
	Vector3<float> position;
	uintptr_t designer_name_ptr;
	char designer_name_buffer[64];
	std::string designer_name;

	bool valid() const {
		return instance != 0 && identity != 0;
	}
};

class SkeletonBone
{
public:
	Vector3<float> position;
	float scale;
	Vector4<float> rotation;
};

class Player
{
public:
	Player(Entity entity) : entity(entity) {}
	Entity entity;

	uintptr_t controller_pawn;
	uintptr_t list_pawn;
	uintptr_t player_pawn;
	uintptr_t bone_array;

#define BONE_COUNT 64
	SkeletonBone bones[BONE_COUNT];

};

std::vector<Entity> getEntities(Core* core)
{
	const int MaxLists = 4;
	const int ListEntries = 512;
	const int CEntityIdentitySize = 120;

	uintptr_t listAddresses[MaxLists];
	core->m_access_adapter->addScatterRead(ent_list + 0x10, listAddresses, sizeof(listAddresses));
	core->m_access_adapter->executeScatterRead();

	std::vector<std::vector<uint8_t>> entityChunks;
	std::vector<uintptr_t> validListAddresses;

	for (int i = 0; i < MaxLists; i++)
	{
		uintptr_t listAddress = listAddresses[i];
		if (listAddress == 0) break;

		validListAddresses.push_back(listAddress);
		entityChunks.emplace_back(ListEntries * CEntityIdentitySize);
	}

	for (size_t i = 0; i < validListAddresses.size(); i++)
	{
		core->m_access_adapter->addScatterRead(validListAddresses[i], entityChunks[i].data(), entityChunks[i].size());
	}

	core->m_access_adapter->executeScatterRead();

	std::vector<Entity> entities;
	entities.resize(validListAddresses.size() * ListEntries);
	for (size_t i = 0; i < entityChunks.size(); i++)
	{
		for (int j = 0; j < ListEntries; j++)
		{
			int entityIndex = static_cast<int>(i) * ListEntries + j;
			if (entityIndex == 0) 
				continue;

			uintptr_t* identity = reinterpret_cast<uintptr_t*>(entityChunks[i].data() + (j * CEntityIdentitySize));
			if (identity[0] == 0) 
				continue;

			Entity newEntity;
			newEntity.instance = identity[0];

			entities.push_back(newEntity);
		}
	}

	for (auto& entity : entities)
	{
		core->m_access_adapter->addScatterRead(entity.instance + 0x10, &entity.identity, sizeof(entity.identity));
		core->m_access_adapter->addScatterRead(entity.instance + 0x330, &entity.game_scene_node, sizeof(entity.game_scene_node));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& entity : entities)
	{
		core->m_access_adapter->addScatterRead(entity.identity + 0x20, &entity.designer_name_ptr, sizeof(entity.designer_name_ptr));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& entity : entities)
	{
		core->m_access_adapter->addScatterRead(entity.designer_name_ptr, &entity.designer_name_buffer, sizeof(entity.designer_name_buffer));
		core->m_access_adapter->addScatterRead(entity.game_scene_node + 0x88, &entity.position, sizeof(entity.position));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& entity : entities)
	{
		entity.designer_name = std::string(entity.designer_name_buffer);
	}

	entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity& e) { return !e.valid(); }), entities.end());

	return entities;
}

void get_players(Core* core)
{
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	std::vector<Entity> entities = getEntities(core);

	std::vector<Player> players;
	for (size_t i = 0; i < entities.size(); i++)
	{
		if (entities[i].designer_name == "cs_player_controller")
		{
			players.push_back(Player(entities[i]));
		}
	}

	for (auto& player : players)
	{
		core->m_access_adapter->addScatterRead(player.entity.instance + 0x8FC, &player.controller_pawn, sizeof(player.controller_pawn));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& player : players)
	{
		core->m_access_adapter->addScatterRead(ent_list + 0x8 * ((player.controller_pawn & 0x7FFF) >> 0X9) + 0x10, &player.list_pawn, sizeof(player.list_pawn));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& player : players)
	{
		core->m_access_adapter->addScatterRead(player.list_pawn + 0x78 * (player.controller_pawn & 0x1FF), &player.player_pawn, sizeof(player.player_pawn));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& player : players)
	{
		core->m_access_adapter->addScatterRead(player.player_pawn + 0x330, &player.entity.game_scene_node, sizeof(player.entity.game_scene_node));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& player : players)
	{
		core->m_access_adapter->addScatterRead(player.entity.game_scene_node + 0x190 + 0x80, &player.bone_array, sizeof(player.bone_array));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& player : players)
	{
		core->m_access_adapter->addScatterRead(player.bone_array, &player.bones, sizeof(player.bones));
	}
	core->m_access_adapter->executeScatterRead();

	for (auto& player : players)
	{
		for (size_t j = 0; j < BONE_COUNT; j++)
		{
			Vector2<float> screen_position;
			if (core->m_projection_utils->WorldToScreen(player.bones[j].position, screen_position, view_matrix))
			{
				draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 3.f, IM_COL32(255, 0, 0, 100), 8);
			}
		}
	}
}

int main()
{
	std::unique_ptr<Core> core = std::make_unique<Core>();

	core->with_target_type(TargetKind::Local)
		.with_logger_backend(LoggerBackend::Console)
		.with_logger_level(LogLevel::Debug)
		.with_target("Counter-Strike 2", nullptr, "cs2.exe")
		.with_window_title("CS2 Cheat");

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
