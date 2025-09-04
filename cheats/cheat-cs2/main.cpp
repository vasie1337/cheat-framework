#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

#include "sdk/sdk.hpp"

// Globals
uintptr_t client_dll = 0x0;
uintptr_t ent_list = 0x0;
uintptr_t local_player_ptr = 0x0;
matrix4x4_t<float> view_matrix;

void get_globals(Core& core)
{
    if (!client_dll)
        client_dll = core.m_access_adapter->get_module("client.dll")->base;

    core.m_access_adapter->add_scatter(client_dll + dwLocalPlayerPawn, &local_player_ptr, sizeof(local_player_ptr));
    core.m_access_adapter->add_scatter(client_dll + dwEntityList, &ent_list, sizeof(ent_list));
    core.m_access_adapter->add_scatter(client_dll + dwViewMatrix, &view_matrix, sizeof(matrix4x4_t<float>));
    core.m_access_adapter->execute_scatter();
}

std::vector<CacheEntity> getEntities(Core& core)
{

    uintptr_t listAddresses[MAX_LISTS];
    core.m_access_adapter->add_scatter(ent_list + 0x10, listAddresses, sizeof(listAddresses));
    core.m_access_adapter->execute_scatter();

    std::vector<std::vector<uint8_t>> entityChunks;
    std::vector<uintptr_t> validListAddresses;

    for (int i = 0; i < MAX_LISTS; i++)
    {
        if (listAddresses[i] == 0) break;
        validListAddresses.push_back(listAddresses[i]);
        entityChunks.emplace_back(LIST_ENTRIES * CENTITY_IDENTITY_SIZE);
    }

    for (size_t i = 0; i < validListAddresses.size(); i++)
    {
        core.m_access_adapter->add_scatter(validListAddresses[i], entityChunks[i].data(), entityChunks[i].size());
    }
    core.m_access_adapter->execute_scatter();

    std::vector<CacheEntity> entities;
    for (int i = 0; i < entityChunks.size(); i++)
    {
        for (int j = 0; j < LIST_ENTRIES; j++)
        {
            int entityIndex = i * LIST_ENTRIES + j;
            if (entityIndex == 0) continue;

            uintptr_t* identity = reinterpret_cast<uintptr_t*>(entityChunks[i].data() + (j * CENTITY_IDENTITY_SIZE));
            if (identity[0] == 0) continue;

            entities.push_back({ identity[0] });
        }
    }

    // Read entity data in batches
    for (auto& entity : entities)
    {
        core.m_access_adapter->add_scatter(entity.instance + CEntityInstance::m_pEntity, &entity.identity, sizeof(entity.identity));
        core.m_access_adapter->add_scatter(entity.instance + C_BaseEntity::m_pGameSceneNode, &entity.game_scene_node, sizeof(entity.game_scene_node));
    }
    core.m_access_adapter->execute_scatter();

    for (auto& entity : entities)
    {
        core.m_access_adapter->add_scatter(entity.identity + CEntityIdentity::m_designerName, &entity.designer_name_ptr, sizeof(entity.designer_name_ptr));
    }
    core.m_access_adapter->execute_scatter();

    for (auto& entity : entities)
    {
        core.m_access_adapter->add_scatter(entity.designer_name_ptr, &entity.designer_name_buffer, sizeof(entity.designer_name_buffer));
        core.m_access_adapter->add_scatter(entity.game_scene_node + CGameSceneNode::m_vecOrigin, &entity.position, sizeof(entity.position));
    }
    core.m_access_adapter->execute_scatter();

    for (auto& entity : entities)
    {
        entity.designer_name = entity.designer_name_buffer;
    }

    entities.erase(std::remove_if(entities.begin(), entities.end(), [](const CacheEntity& e) { return !e.valid(); }), entities.end());
    return entities;
}

void get_players(Core& core)
{
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    std::vector<CacheEntity> entities = getEntities(core);

    std::vector<CachePlayer> players;
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
        core.m_access_adapter->add_scatter(player.entity.instance + CCSPlayerController::m_hPlayerPawn, &player.controller_pawn, sizeof(player.controller_pawn));
    }
    core.m_access_adapter->execute_scatter();

    // Read list pawns
    for (auto& player : players)
    {
        core.m_access_adapter->add_scatter(ent_list + 0x8 * ((player.controller_pawn & 0x7FFF) >> 0X9) + 0x10, &player.list_pawn, sizeof(player.list_pawn));
    }
    core.m_access_adapter->execute_scatter();

    // Read player pawns
    for (auto& player : players)
    {
        core.m_access_adapter->add_scatter(player.list_pawn + 0x78 * (player.controller_pawn & 0x1FF), &player.player_pawn, sizeof(player.player_pawn));
    }
    core.m_access_adapter->execute_scatter();

    // Read game scene nodes
    for (auto& player : players)
    {
        core.m_access_adapter->add_scatter(player.player_pawn + C_BaseEntity::m_pGameSceneNode, &player.entity.game_scene_node, sizeof(player.entity.game_scene_node));
    }
    core.m_access_adapter->execute_scatter();

    // Read bone arrays
    for (auto& player : players)
    {
        core.m_access_adapter->add_scatter(player.entity.game_scene_node + CSkeletonInstance::m_modelState + 0x80, &player.bone_array, sizeof(player.bone_array));
    }
    core.m_access_adapter->execute_scatter();

    // Read bones
    for (auto& player : players)
    {
        core.m_access_adapter->add_scatter(player.bone_array, &player.bones, sizeof(player.bones));
    }
    core.m_access_adapter->execute_scatter();

    // Draw bones
    for (const auto& player : players)
    {
        for (const auto& bone : player.bones)
        {
            vec2_t<float> screen_position;
            if (core.m_projection_utils->WorldToScreen(bone.position, screen_position, view_matrix))
            {
                draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 3.f, IM_COL32(255, 0, 0, 100), 8);
            }
        }
    }
}

// Will read everything with in function time boundaries
// will use a double buffer cache to store data
static void reader(Core& core) {

}

// read from double buffer
// Will render and 2ws cached positions from player bones and ent positions
static void renderer(Core& core) {

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

    //core->register_function(get_globals, 1000);
    //core->register_function(get_players, 1);

    core->register_function(reader, 1000);
    core->register_function(renderer, -1);

    while (core->update()) {}
    return 0;
}