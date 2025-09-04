#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>
#include <chrono>
#include <unordered_set>

#include "sdk/sdk.hpp"

struct CachedEntity {
    uintptr_t instance = 0;
    uintptr_t identity = 0;
    uintptr_t game_scene_node = 0;
    uintptr_t designer_name_ptr = 0;
    char designer_name_buffer[128] = {};
    std::string designer_name;
    vec3_t<float> position;
    bool data_cached = false;
    
    bool valid() const {
        return instance != 0 && !designer_name.empty(); 
    }
};

struct CachedPlayer {
    CachedEntity entity;
    uintptr_t controller_pawn = 0;
    uintptr_t list_pawn = 0;
    uintptr_t player_pawn = 0;
    uintptr_t bone_array = 0;
    std::array<SkeletonBone, MAX_BONES> bones;
    bool data_cached = false;
};

struct GameCache {
    uintptr_t client_dll = 0x0;
    uintptr_t ent_list = 0x0;
    uintptr_t local_player_ptr = 0x0;
    mat4f view_matrix;

    // Cached entities with pointer tracking
    std::vector<uintptr_t> entity_pointers;
    std::vector<CachedEntity> entities;
    std::vector<CachedPlayer> players;
    
    // Cache timestamps
    std::chrono::steady_clock::time_point last_globals_update;
    std::chrono::steady_clock::time_point last_entity_scan;
    
    // Cache intervals (in milliseconds)
    static constexpr int GLOBALS_INTERVAL = 1000;  // 1 second
    static constexpr int ENTITY_SCAN_INTERVAL = 100; // 100ms
} game_cache;

bool should_update_globals() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - game_cache.last_globals_update).count();
    return elapsed >= GameCache::GLOBALS_INTERVAL;
}

bool should_scan_entities() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - game_cache.last_entity_scan).count();
    return elapsed >= GameCache::ENTITY_SCAN_INTERVAL;
}

void read_globals(Core& core) {
    // Always update view matrix for smooth rendering
    core.m_access_adapter->add_scatter(game_cache.client_dll + dwViewMatrix, 
        &game_cache.view_matrix, sizeof(game_cache.view_matrix));
    
    // Only update other globals periodically
    if (should_update_globals()) {
        core.m_access_adapter->add_scatter(game_cache.client_dll + dwLocalPlayerPawn, 
            &game_cache.local_player_ptr, sizeof(game_cache.local_player_ptr));
        core.m_access_adapter->add_scatter(game_cache.client_dll + dwEntityList, 
            &game_cache.ent_list, sizeof(game_cache.ent_list));
        game_cache.last_globals_update = std::chrono::steady_clock::now();
    }
    
    core.m_access_adapter->execute_scatter();
}

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

bool pointers_changed(const std::vector<uintptr_t>& old_ptrs, 
                     const std::vector<uintptr_t>& new_ptrs) {
    if (old_ptrs.size() != new_ptrs.size()) return true;
    
    std::unordered_set<uintptr_t> old_set(old_ptrs.begin(), old_ptrs.end());
    for (uintptr_t ptr : new_ptrs) {
        if (old_set.find(ptr) == old_set.end()) {
            return true;
        }
    }
    return false;
}

void update_entity_cache(Core& core, const std::vector<uintptr_t>& new_pointers) {
    // Create new entity cache
    std::vector<CachedEntity> new_entities;
    
    // Try to preserve existing cached data
    std::unordered_map<uintptr_t, CachedEntity> old_cache;
    for (const auto& entity : game_cache.entities) {
        old_cache[entity.instance] = entity;
    }
    
    // Build new entity list, preserving cached data where possible
    for (uintptr_t instance : new_pointers) {
        CachedEntity entity;
        entity.instance = instance;
        
        auto it = old_cache.find(instance);
        if (it != old_cache.end() && it->second.data_cached) {
            // Reuse cached data
            entity = it->second;
        }
        new_entities.push_back(entity);
    }
    
    game_cache.entities = std::move(new_entities);
    
    // Read missing data for entities that aren't cached
    std::vector<CachedEntity*> uncached_entities;
    for (auto& entity : game_cache.entities) {
        if (!entity.data_cached) {
            uncached_entities.push_back(&entity);
        }
    }
    
    if (!uncached_entities.empty()) {
        // Read basic entity data
        for (auto* entity : uncached_entities) {
            core.m_access_adapter->add_scatter(entity->instance + CEntityInstance::m_pEntity, 
                &entity->identity, sizeof(entity->identity));
            core.m_access_adapter->add_scatter(entity->instance + C_BaseEntity::m_pGameSceneNode, 
                &entity->game_scene_node, sizeof(entity->game_scene_node));
        }
        core.m_access_adapter->execute_scatter();

        // Read designer names
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

        // Finalize cached entities
        for (auto* entity : uncached_entities) {
            entity->designer_name = entity->designer_name_buffer;
            entity->data_cached = true;
        }
    }
}

void update_entity_positions(Core& core) {
    // Always update positions for smooth movement
    for (auto& entity : game_cache.entities) {
        if (entity.data_cached && entity.game_scene_node != 0) {
            core.m_access_adapter->add_scatter(entity.game_scene_node + CGameSceneNode::m_vecOrigin, 
                &entity.position, sizeof(entity.position));
        }
    }
    core.m_access_adapter->execute_scatter();
}

void update_player_cache(Core& core) {
    std::vector<CachedPlayer> new_players;
    
    // Create map of existing player data
    std::unordered_map<uintptr_t, CachedPlayer> old_player_cache;
    for (const auto& player : game_cache.players) {
        old_player_cache[player.entity.instance] = player;
    }
    
    // Find player controllers from entities
    for (const auto& entity : game_cache.entities) {
        if (entity.designer_name == "cs_player_controller") {
            CachedPlayer player;
            player.entity = entity;
            
            // Try to preserve cached player data
            auto it = old_player_cache.find(entity.instance);
            if (it != old_player_cache.end() && it->second.data_cached) {
                player = it->second;
                player.entity = entity; // Update entity reference
            }
            
            new_players.push_back(player);
        }
    }
    
    game_cache.players = std::move(new_players);
    
    // Read missing player data
    std::vector<CachedPlayer*> uncached_players;
    for (auto& player : game_cache.players) {
        if (!player.data_cached) {
            uncached_players.push_back(&player);
        }
    }
    
    if (!uncached_players.empty()) {
        // Read controller pawns
        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->entity.instance + CCSPlayerController::m_hPlayerPawn, 
                &player->controller_pawn, sizeof(player->controller_pawn));
        }
        core.m_access_adapter->execute_scatter();

        // Read list pawns
        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(game_cache.ent_list + 0x8 * ((player->controller_pawn & 0x7FFF) >> 0X9) + 0x10, 
                &player->list_pawn, sizeof(player->list_pawn));
        }
        core.m_access_adapter->execute_scatter();

        // Read player pawns
        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->list_pawn + 0x78 * (player->controller_pawn & 0x1FF), 
                &player->player_pawn, sizeof(player->player_pawn));
        }
        core.m_access_adapter->execute_scatter();

        // Read game scene nodes
        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->player_pawn + C_BaseEntity::m_pGameSceneNode, 
                &player->entity.game_scene_node, sizeof(player->entity.game_scene_node));
        }
        core.m_access_adapter->execute_scatter();

        // Read bone arrays
        for (auto* player : uncached_players) {
            core.m_access_adapter->add_scatter(player->entity.game_scene_node + CSkeletonInstance::m_modelState + 0x80, 
                &player->bone_array, sizeof(player->bone_array));
        }
        core.m_access_adapter->execute_scatter();
        
        // Mark as cached
        for (auto* player : uncached_players) {
            player->data_cached = true;
        }
    }
}

void update_player_bones(Core& core) {
    // Always update bones for smooth animation
    for (auto& player : game_cache.players) {
        if (player.data_cached && player.bone_array != 0) {
            core.m_access_adapter->add_scatter(player.bone_array, 
                &player.bones, sizeof(player.bones));
        }
    }
    core.m_access_adapter->execute_scatter();
}

static void reader(Core& core) {
    read_globals(core);
    
    // Check if we need to scan for entity changes
    if (should_scan_entities()) {
        auto new_pointers = scan_entity_pointers(core);
        
        if (pointers_changed(game_cache.entity_pointers, new_pointers)) {
            log_debug("Entity list changed, updating cache");
            game_cache.entity_pointers = new_pointers;
            update_entity_cache(core, new_pointers);
            update_player_cache(core);
        }
        
        game_cache.last_entity_scan = std::chrono::steady_clock::now();
    }
    
    // Always update dynamic data
    update_entity_positions(core);
    update_player_bones(core);
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

    core->with_target_type(TargetKind::Local)
        .with_logger_backend(LoggerBackend::Console)
        .with_logger_level(LogLevel::Debug)
        .with_target("Counter-Strike 2", "cs2.exe")
        .with_window_title("CS2 Cheat");

    if (!core->initialize()) {
        log_critical("Failed to initialize core");
        return 1;
    }

    game_cache.client_dll = core->m_access_adapter->get_module("client.dll")->base;
    
    // Initialize cache timestamps
    game_cache.last_globals_update = std::chrono::steady_clock::now();
    game_cache.last_entity_scan = std::chrono::steady_clock::now();

    core->register_function(reader);
    core->register_function(renderer);

    while (core->update()) {}
    return 0;
}