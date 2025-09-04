#pragma once
#include <core/core.hpp>
#include <sdk/sdk.hpp>

struct GameCache {
    uint32_t game_base = 0;
    uint32_t player_list_ptr = 0;
    int player_list_count = 0;
    matrix4x4_t<float> view_matrix;
    std::vector<uint32_t> last_player_pointers;
    std::vector<CachedPlayer> cached_players;

    static constexpr int GLOBALS_INTERVAL = 1000;
    static constexpr int PLAYER_SCAN_INTERVAL = 50;
} game_cache;

static void cache_static_player_data(Core& core) {
    std::vector<CachedPlayer*> uncached_players;
    for (auto& player : game_cache.cached_players) {
        if (!player.data_cached && player.pointer != 0) {
            uncached_players.push_back(&player);
        }
    }

    if (uncached_players.empty()) return;

    for (auto* player : uncached_players) {
        core.m_access_adapter->add_scatter(player->pointer + 0x178,
            &player->health, sizeof(int));
        core.m_access_adapter->add_scatter(player->pointer + 0x32C,
            &player->team, sizeof(int));
    }

    core.m_access_adapter->execute_scatter();

    for (auto* player : uncached_players) {
        player->data_cached = true;
    }
}

static void update_player_cache(const std::vector<uint32_t>& new_pointers, Core& core) {
    std::unordered_map<uint32_t, CachedPlayer> old_cache;
    for (const auto& player : game_cache.cached_players) {
        old_cache[player.pointer] = player;
    }

    std::vector<CachedPlayer> new_players;
    for (uint32_t pointer : new_pointers) {
        CachedPlayer player;
        player.pointer = pointer;

        auto it = old_cache.find(pointer);
        if (it != old_cache.end() && it->second.data_cached) {
            player = it->second;
        }
        new_players.push_back(player);
    }

    game_cache.cached_players = std::move(new_players);
    cache_static_player_data(core);
}

static void update_player_list(Core& core) {
    if (game_cache.player_list_count <= 1) return;

    std::vector<uint32_t> current_pointers(game_cache.player_list_count);
    for (int i = 0; i < game_cache.player_list_count; ++i) {
        core.m_access_adapter->add_scatter(
            game_cache.player_list_ptr + i * sizeof(uint32_t),
            &current_pointers[i], sizeof(uint32_t));
    }
    core.m_access_adapter->execute_scatter();

    if (has_changed(game_cache.last_player_pointers, current_pointers)) {
        log_debug("Player list changed, updating cache");
        game_cache.last_player_pointers = current_pointers;
        update_player_cache(current_pointers, core);
    }
}

static void reader(Core& core) {
    if (core.m_update_manager->should_update("globals", game_cache.GLOBALS_INTERVAL)) {
        core.m_access_adapter->add_scatter(game_cache.game_base + 0x18AC04, &game_cache.player_list_ptr, sizeof(uint32_t));
        core.m_access_adapter->add_scatter(game_cache.game_base + 0x18AC0C, &game_cache.player_list_count, sizeof(int));
    }

    for (auto& player : game_cache.cached_players) {
        if (player.pointer != 0) {
            core.m_access_adapter->add_scatter(
                player.pointer + 0x4,
                &player.position, sizeof(vec3_t<float>));
        }
    }

    core.m_access_adapter->add_scatter(game_cache.game_base + 0x17DFD0, &game_cache.view_matrix, sizeof(matrix4x4_t<float>));
    core.m_access_adapter->execute_scatter();

    if (core.m_update_manager->should_update("player_scan", game_cache.PLAYER_SCAN_INTERVAL)) {
        update_player_list(core);
    }

    core.m_access_adapter->execute_scatter();
}

static void renderer(Core& core) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    for (const auto& player : game_cache.cached_players) {
        if (player.pointer == 0) continue;

        vec2_t<float> screen_position;
        if (core.m_projection_utils->WorldToScreenDX(player.position, screen_position, game_cache.view_matrix)) {

            draw_list->AddCircleFilled(ImVec2(screen_position.x, screen_position.y), 5.f, IM_COL32(255, 0, 0, 255), 8);
        }
    }
}

int main() {
    std::unique_ptr<Core> core = std::make_unique<Core>();

    core->with_target_type(TargetType::Local)
        .with_logger_backend(LoggerBackend::Console)
        .with_logger_level(LogLevel::Debug)
        .with_target("AssaultCube", "ac_client.exe")
        .with_window_title("AssaultCube Cheat");

    if (!core->initialize()) {
        log_critical("Failed to initialize core");
        return 1;
    }

    game_cache.game_base = static_cast<uint32_t>(core->m_access_adapter->get_module("ac_client.exe")->base);

    core->m_update_manager->force_update_all();

    core->register_function(reader);
    core->register_function(renderer);

    while (core->update()) {}
    return 0;
}