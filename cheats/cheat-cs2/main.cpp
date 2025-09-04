#include <core/core.hpp>
#include <sdk/sdk.hpp>

struct GameCache {
    uintptr_t client_dll = 0x0;
    uintptr_t ent_list = 0x0;
    uintptr_t local_player_ptr = 0x0;
    mat4f view_matrix;

    std::vector<uintptr_t> entity_pointers;
    std::vector<CachedEntity> entities;
    std::vector<CachedPlayer> players;

    static constexpr int GLOBALS_INTERVAL = 1000;
    static constexpr int ENTITY_SCAN_INTERVAL = 50;
} game_cache;

std::vector<uintptr_t> scan_entity_pointers(Core& core) {
    uintptr_t list_ptrs[MAX_LISTS]{};
    core.m_access_adapter->add_scatter(game_cache.ent_list + 0x10, list_ptrs, sizeof(list_ptrs));
    core.m_access_adapter->execute_scatter();

    std::vector<std::vector<uint8_t>> entity_chunks;
    std::vector<uintptr_t> valid_list_ptrs;

    for (int i = 0; i < MAX_LISTS; i++) {
        if (list_ptrs[i] == 0) break;
        valid_list_ptrs.push_back(list_ptrs[i]);
        entity_chunks.emplace_back(LIST_ENTRIES * CENTITY_IDENTITY_SIZE);
    }

    for (size_t i = 0; i < valid_list_ptrs.size(); i++) {
        core.m_access_adapter->add_scatter(valid_list_ptrs[i],
            entity_chunks[i].data(), entity_chunks[i].size());
    }
    core.m_access_adapter->execute_scatter();

    std::vector<uintptr_t> entity_pointers;
    for (int i = 0; i < entity_chunks.size(); i++) {
        for (int j = 0; j < LIST_ENTRIES; j++) {
            int entityIndex = i * LIST_ENTRIES + j;
            if (entityIndex == 0) continue;

            uintptr_t* identity = reinterpret_cast<uintptr_t*>(
                entity_chunks[i].data() + (j * CENTITY_IDENTITY_SIZE));
            if (identity[0] != 0) {
                entity_pointers.push_back(identity[0]);
            }
        }
    }

    return entity_pointers;
}

void update_entity_cache(Core& core, const std::vector<uintptr_t>& new_pointers) {
    std::vector<CachedEntity> new_entities;

    std::unordered_map<uintptr_t, CachedEntity> old_cache;
    for (const auto& entity : game_cache.entities) {
        old_cache[entity.instance] = entity;
    }

    for (uintptr_t instance : new_pointers) {
        CachedEntity entity;
        entity.instance = instance;

        auto it = old_cache.find(instance);
        if (it != old_cache.end() && it->second.data_cached) {
            entity = it->second;
        }
        new_entities.push_back(entity);
    }

    game_cache.entities = std::move(new_entities);

    std::vector<CachedEntity*> uncached_entities;
    for (auto& entity : game_cache.entities) {
        if (!entity.data_cached) {
            uncached_entities.push_back(&entity);
        }
    }

    if (!uncached_entities.empty()) {
        for (auto* entity : uncached_entities) {
            core.m_access_adapter->add_scatter(entity->instance + CEntityInstance::m_pEntity,
                &entity->identity, sizeof(entity->identity));
            core.m_access_adapter->add_scatter(entity->instance + C_BaseEntity::m_pGameSceneNode,
                &entity->game_scene_node, sizeof(entity->game_scene_node));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* entity : uncached_entities) {
            core.m_access_adapter->add_scatter(entity->identity + CEntityIdentity::m_designerName,
                &entity->designer_name_ptr, sizeof(entity->designer_name_ptr));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* entity : uncached_entities) {
            core.m_access_adapter->add_scatter(entity->designer_name_ptr,
                &entity->designer_name_buffer, sizeof(entity->designer_name_buffer));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* entity : uncached_entities) {
            entity->designer_name = entity->designer_name_buffer;
            entity->data_cached = true;
        }
    }
}

void update_player_cache(Core& core) {
    std::vector<CachedPlayer> new_players;

    std::unordered_map<uintptr_t, CachedPlayer> old_player_cache;
    for (const auto& player : game_cache.players) {
        old_player_cache[player.entity.instance] = player;
    }

    for (const auto& entity : game_cache.entities) {
        if (entity.designer_name == "cs_player_controller") {
            CachedPlayer player;
            player.entity = entity;

            auto it = old_player_cache.find(entity.instance);
            if (it != old_player_cache.end() && it->second.data_cached) {
                player = it->second;
                player.entity = entity;
            }

            new_players.push_back(player);
        }
    }

    game_cache.players = std::move(new_players);

    std::vector<CachedPlayer*> uncached_players;
    for (auto& player : game_cache.players) {
        if (!player.data_cached) {
            uncached_players.push_back(&player);
        }
    }

    if (!uncached_players.empty()) {
        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->entity.instance + CCSPlayerController::m_hPlayerPawn,
                &player->controller_pawn, sizeof(player->controller_pawn));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(game_cache.ent_list + 0x8 * ((player->controller_pawn & 0x7FFF) >> 0X9) + 0x10,
                &player->list_pawn, sizeof(player->list_pawn));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->list_pawn + 0x78 * (player->controller_pawn & 0x1FF),
                &player->player_pawn, sizeof(player->player_pawn));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->player_pawn + C_BaseEntity::m_pGameSceneNode,
                &player->entity.game_scene_node, sizeof(player->entity.game_scene_node));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->entity.game_scene_node + CSkeletonInstance::m_modelState + 0x80,
                &player->bone_array, sizeof(player->bone_array));
        }
        core.m_access_adapter->execute_scatter();

        for (auto* player : uncached_players) {
            player->data_cached = true;
        }
    }
}

static void reader(Core& core) {
    core.m_access_adapter->add_scatter(game_cache.client_dll + dwViewMatrix,
        &game_cache.view_matrix, sizeof(game_cache.view_matrix));

    if (core.m_update_manager->should_update("globals", GameCache::GLOBALS_INTERVAL)) {
        core.m_access_adapter->add_scatter(game_cache.client_dll + dwLocalPlayerPawn,
            &game_cache.local_player_ptr, sizeof(game_cache.local_player_ptr));
        core.m_access_adapter->add_scatter(game_cache.client_dll + dwEntityList,
            &game_cache.ent_list, sizeof(game_cache.ent_list));
    }

    core.m_access_adapter->execute_scatter();

    if (core.m_update_manager->should_update("entity_scan", GameCache::ENTITY_SCAN_INTERVAL)) {
        auto new_pointers = scan_entity_pointers(core);

        if (has_changed(game_cache.entity_pointers, new_pointers)) {
            log_debug("Entity list changed, updating cache");
            game_cache.entity_pointers = new_pointers;
            update_entity_cache(core, new_pointers);
            update_player_cache(core);
        }
    }

    for (auto& entity : game_cache.entities) {
        if (entity.data_cached && entity.game_scene_node != 0) {
            core.m_access_adapter->add_scatter(entity.game_scene_node + CGameSceneNode::m_vecOrigin,
                &entity.position, sizeof(entity.position));
        }
    }

    for (auto& player : game_cache.players) {
        if (player.data_cached && player.bone_array != 0) {
            core.m_access_adapter->add_scatter(player.bone_array,
                &player.bones, sizeof(player.bones));
        }
    }
    core.m_access_adapter->execute_scatter();
}

static void renderer(Core& core) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    for (const auto& player : game_cache.players) {
        if (!player.data_cached) continue;

        for (const auto& bone : player.bones) {
            vec2_t<float> screen_position;
            if (core.m_projection_utils->WorldToScreen(bone.position, screen_position, game_cache.view_matrix)) {
                draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 3.f, IM_COL32(255, 0, 0, 100), 8);
            }
        }
    }

    for (const auto& entity : game_cache.entities) {
        if (!entity.data_cached || entity.designer_name == "cs_player_controller") continue;

        vec2_t<float> screen_position;
        if (core.m_projection_utils->WorldToScreen(entity.position, screen_position, game_cache.view_matrix)) {
            draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 2.f, IM_COL32(0, 255, 0, 100), 6);
        }
    }
}

int main() {
    auto core = std::make_unique<Core>();

    core->with_target_type(TargetType::Local)
        .with_logger_backend(LoggerBackend::Console)
        .with_logger_level(LogLevel::Debug)
        .with_target("Counter-Strike 2", "cs2.exe")
        .with_window_title("CS2 Cheat");

    if (!core->initialize()) {
        log_critical("Failed to initialize core");
        return 1;
    }

    game_cache.client_dll = core->m_access_adapter->get_module("client.dll")->base;

    core->m_update_manager->force_update_all();

    core->register_function(reader);
    core->register_function(renderer);

    while (core->update()) {}
    return 0;
}