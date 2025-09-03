#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

// Globals
uintptr_t client_dll = 0x0;
uintptr_t ent_list = 0x0;
uintptr_t local_player_ptr = 0x0;
matrix4x4_t<float> view_matrix;

void get_globals(Core* core)
{
    if (!client_dll)
        client_dll = core->m_access_adapter->getModule("client.dll")->baseAddress;

    core->m_access_adapter->addScatterRead(client_dll + 0x1BEC440, &local_player_ptr, sizeof(local_player_ptr));
    core->m_access_adapter->addScatterRead(client_dll + 0x1D0FE08, &ent_list, sizeof(ent_list));
    core->m_access_adapter->addScatterRead(client_dll + 0x1E2D030, &view_matrix, sizeof(matrix4x4_t<float>));
    core->m_access_adapter->executeScatterRead();
}

struct Entity
{
    uintptr_t instance = 0;
    uintptr_t identity = 0;
    uintptr_t game_scene_node = 0;
    vec3_t<float> position;
    uintptr_t designer_name_ptr = 0;
    char designer_name_buffer[64] = {};
    std::string designer_name;

    bool valid() const { return instance != 0 && identity != 0; }
};

struct SkeletonBone
{
    vec3_t<float> position;
    float scale;
    vec4_t<float> rotation;
};

struct Player
{
    Entity entity;
    uintptr_t controller_pawn = 0;
    uintptr_t list_pawn = 0;
    uintptr_t player_pawn = 0;
    uintptr_t bone_array = 0;
    SkeletonBone bones[64];
};

std::vector<Entity> getEntities(Core* core)
{
    constexpr int MaxLists = 0x4;
    constexpr int ListEntries = 0x200;
    constexpr int CEntityIdentitySize = 0x78;

    uintptr_t listAddresses[MaxLists];
    core->m_access_adapter->addScatterRead(ent_list + 0x10, listAddresses, sizeof(listAddresses));
    core->m_access_adapter->executeScatterRead();

    std::vector<std::vector<uint8_t>> entityChunks;
    std::vector<uintptr_t> validListAddresses;

    for (int i = 0; i < MaxLists; i++)
    {
        if (listAddresses[i] == 0) break;
        validListAddresses.push_back(listAddresses[i]);
        entityChunks.emplace_back(ListEntries * CEntityIdentitySize);
    }

    for (size_t i = 0; i < validListAddresses.size(); i++)
    {
        core->m_access_adapter->addScatterRead(validListAddresses[i], entityChunks[i].data(), entityChunks[i].size());
    }
    core->m_access_adapter->executeScatterRead();

    std::vector<Entity> entities;
    for (int i = 0; i < entityChunks.size(); i++)
    {
        for (int j = 0; j < ListEntries; j++)
        {
            int entityIndex = i * ListEntries + j;
            if (entityIndex == 0) continue;

            uintptr_t* identity = reinterpret_cast<uintptr_t*>(entityChunks[i].data() + (j * CEntityIdentitySize));
            if (identity[0] == 0) continue;

            entities.push_back({ identity[0] });
        }
    }

    // Read entity data in batches
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
        entity.designer_name = entity.designer_name_buffer;
    }

    entities.erase(std::remove_if(entities.begin(), entities.end(), [](const Entity& e) { return !e.valid(); }), entities.end());
    return entities;
}

void get_players(Core* core)
{
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    std::vector<Entity> entities = getEntities(core);

    std::vector<Player> players;
    for (const auto& entity : entities)
    {
        if (entity.designer_name == "cs_player_controller")
        {
            players.push_back({ entity });
        }
    }

    // Read controller pawns
    for (auto& player : players)
    {
        core->m_access_adapter->addScatterRead(player.entity.instance + 0x8FC, &player.controller_pawn, sizeof(player.controller_pawn));
    }
    core->m_access_adapter->executeScatterRead();

    // Read list pawns
    for (auto& player : players)
    {
        core->m_access_adapter->addScatterRead(ent_list + 0x8 * ((player.controller_pawn & 0x7FFF) >> 0X9) + 0x10, &player.list_pawn, sizeof(player.list_pawn));
    }
    core->m_access_adapter->executeScatterRead();

    // Read player pawns
    for (auto& player : players)
    {
        core->m_access_adapter->addScatterRead(player.list_pawn + 0x78 * (player.controller_pawn & 0x1FF), &player.player_pawn, sizeof(player.player_pawn));
    }
    core->m_access_adapter->executeScatterRead();

    // Read game scene nodes
    for (auto& player : players)
    {
        core->m_access_adapter->addScatterRead(player.player_pawn + 0x330, &player.entity.game_scene_node, sizeof(player.entity.game_scene_node));
    }
    core->m_access_adapter->executeScatterRead();

    // Read bone arrays
    for (auto& player : players)
    {
        core->m_access_adapter->addScatterRead(player.entity.game_scene_node + 0x190 + 0x80, &player.bone_array, sizeof(player.bone_array));
    }
    core->m_access_adapter->executeScatterRead();

    // Read bones
    for (auto& player : players)
    {
        core->m_access_adapter->addScatterRead(player.bone_array, &player.bones, sizeof(player.bones));
    }
    core->m_access_adapter->executeScatterRead();

    // Draw bones
    for (const auto& player : players)
    {
        for (const auto& bone : player.bones)
        {
            vec2_t<float> screen_position;
            if (core->m_projection_utils->WorldToScreen(bone.position, screen_position, view_matrix))
            {
                draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 3.f, IM_COL32(255, 0, 0, 100), 8);
            }
        }
    }
}

int main()
{
    auto core = std::make_unique<Core>();

    core->with_target_type(TargetKind::Local)
        .with_logger_backend(LoggerBackend::Console)
        .with_logger_level(LogLevel::Debug)
        .with_target("Counter-Strike 2", "cs2.exe")
        .with_window_title("CS2 Cheat");

    if (!core->initialize())
    {
        log_critical("Failed to initialize core");
        return 1;
    }

    core->register_callback(get_globals);
    core->register_callback(get_players);

    while (core->update()) {}
    return 0;
}