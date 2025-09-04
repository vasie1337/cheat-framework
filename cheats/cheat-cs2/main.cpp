#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

#include "sdk/sdk.hpp"

struct GameCache {
    uintptr_t client_dll = 0x0;
    uintptr_t ent_list = 0x0;
    uintptr_t local_player_ptr = 0x0;

    std::vector<CachePlayer> players;
    std::vector<CacheEntity> entities;
    matrix4x4_t<float> view_matrix;
} game_cache;

void read_globals(Core& core)
{
    core.m_access_adapter->add_scatter(game_cache.client_dll + dwLocalPlayerPawn, &game_cache.local_player_ptr, sizeof(game_cache.local_player_ptr));
    core.m_access_adapter->add_scatter(game_cache.client_dll + dwEntityList, &game_cache.ent_list, sizeof(game_cache.ent_list));
    core.m_access_adapter->add_scatter(game_cache.client_dll + dwViewMatrix, &game_cache.view_matrix, sizeof(game_cache.view_matrix));
    core.m_access_adapter->execute_scatter();
}

static std::vector<CacheEntity> read_entities(Core& core)
{
    uintptr_t list_ptrs[MAX_LISTS]{};
    core.m_access_adapter->add_scatter(game_cache.ent_list + 0x10, list_ptrs, sizeof(list_ptrs));
    core.m_access_adapter->execute_scatter();

    std::vector<std::vector<uint8_t>> entity_chunks;
    std::vector<uintptr_t> valid_list_ptrs;

    for (int i = 0; i < MAX_LISTS; i++)
    {
        if (list_ptrs[i] == 0) break;
        valid_list_ptrs.push_back(list_ptrs[i]);
        entity_chunks.emplace_back(LIST_ENTRIES * CENTITY_IDENTITY_SIZE);
    }

    for (size_t i = 0; i < valid_list_ptrs.size(); i++)
    {
        core.m_access_adapter->add_scatter(valid_list_ptrs[i], entity_chunks[i].data(), entity_chunks[i].size());
    }
    core.m_access_adapter->execute_scatter();

    std::vector<CacheEntity> entities;
    for (int i = 0; i < entity_chunks.size(); i++)
    {
        for (int j = 0; j < LIST_ENTRIES; j++)
        {
            int entityIndex = i * LIST_ENTRIES + j;
            if (entityIndex == 0) continue;

            uintptr_t* identity = reinterpret_cast<uintptr_t*>(entity_chunks[i].data() + (j * CENTITY_IDENTITY_SIZE));
            if (identity[0] == 0) continue;

            entities.push_back({ identity[0] });
        }
    }

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

static std::vector<CachePlayer> read_players(Core& core, const std::vector<CacheEntity>& entities)
{
    std::vector<CachePlayer> players;

    // Filter player controllers from entities
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
        core.m_access_adapter->add_scatter(game_cache.ent_list + 0x8 * ((player.controller_pawn & 0x7FFF) >> 0X9) + 0x10, &player.list_pawn, sizeof(player.list_pawn));
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

    return players;
}

static void reader(Core& core)
{
    read_globals(core);

    game_cache.entities = read_entities(core);
    game_cache.players = read_players(core, game_cache.entities);
}

static void renderer(Core& core)
{
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    for (const auto& player : game_cache.players)
    {
        for (const auto& bone : player.bones)
        {
            vec2_t<float> screen_position;
            if (core.m_projection_utils->WorldToScreen(bone.position, screen_position, game_cache.view_matrix))
            {
                draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 3.f, IM_COL32(255, 0, 0, 100), 8);
            }
        }
    }

    for (const auto& entity : game_cache.entities)
    {
        if (entity.designer_name == "cs_player_controller") continue;

        vec2_t<float> screen_position;
        if (core.m_projection_utils->WorldToScreen(entity.position, screen_position, game_cache.view_matrix))
        {
            draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 2.f, IM_COL32(0, 255, 0, 100), 6);

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

    game_cache.client_dll = core->m_access_adapter->get_module("client.dll")->base;

    core->register_function(reader);
    core->register_function(renderer);

    while (core->update()) {}
    return 0;
}