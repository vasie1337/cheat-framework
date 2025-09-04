// assault_cube_cache.hpp
#pragma once
#include <core/core.hpp>
#include <core/logger/logger.hpp>
#include <core/types/vector.hpp>
#include <core/types/matrix.hpp>

struct CachedPlayer {
    uint32_t pointer = 0;
    vec3_t<float> position;
    int health = 0;
    int team = 0;

    bool data_cached = false;
};

static uint32_t game_base_ = 0;
static uint32_t player_list_ptr_ = 0;
static int player_list_count_ = 0;
static matrix4x4_t<float> view_matrix_;
static std::vector<uint32_t> last_player_pointers_;
static std::vector<CachedPlayer> cached_players_;

static constexpr int GLOBALS_INTERVAL = 1000;
static constexpr int PLAYER_SCAN_INTERVAL = 100;

static void update_globals(Core& core) {
    if (!game_base_) {
        game_base_ = static_cast<uint32_t>(
            core.m_access_adapter->get_module("ac_client.exe")->base);
    }

    core.m_access_adapter->add_scatter(game_base_ + 0x18AC04,
        &player_list_ptr_, sizeof(uint32_t));
    core.m_access_adapter->add_scatter(game_base_ + 0x18AC0C,
        &player_list_count_, sizeof(int));
    core.m_access_adapter->execute_scatter();
}

static void cache_static_player_data(Core& core) {
    std::vector<CachedPlayer*> uncached_players;
    for (auto& player : cached_players_) {
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
    for (const auto& player : cached_players_) {
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

    cached_players_ = std::move(new_players);
    cache_static_player_data(core);
}

static void update_player_list(Core& core) {
    if (player_list_count_ <= 1) return;

    std::vector<uint32_t> current_pointers(player_list_count_);
    for (int i = 0; i < player_list_count_; ++i) {
        core.m_access_adapter->add_scatter(
            player_list_ptr_ + i * sizeof(uint32_t),
            &current_pointers[i], sizeof(uint32_t));
    }
    core.m_access_adapter->execute_scatter();

    if (has_changed(last_player_pointers_, current_pointers)) {
        log_debug("Player list changed, updating cache");
        last_player_pointers_ = current_pointers;
        update_player_cache(current_pointers, core);
    }
}

static void update_player_positions(Core& core) {
    for (auto& player : cached_players_) {
        if (player.pointer != 0) {
            core.m_access_adapter->add_scatter(
                player.pointer + 0x4,
                &player.position, sizeof(vec3_t<float>));
        }
    }

    if (!cached_players_.empty()) {
        core.m_access_adapter->execute_scatter();
    }
}

static void update_cache(Core& core) {
    if (core.m_update_manager->should_update("globals", GLOBALS_INTERVAL)) {
        update_globals(core);
    }

    core.m_access_adapter->add_scatter(game_base_ + 0x17DFD0, &view_matrix_, sizeof(matrix4x4_t<float>));
    core.m_access_adapter->execute_scatter();

    if (core.m_update_manager->should_update("player_scan", PLAYER_SCAN_INTERVAL)) {
        update_player_list(core);
    }

    update_player_positions(core);
}

static const std::vector<CachedPlayer>& get_players() {
    return cached_players_;
}

static const matrix4x4_t<float>& get_view_matrix() {
    return view_matrix_;
}

static int get_player_count() {
    return player_list_count_;
}

static void reader(Core& core) {
    update_cache(core);
}

static void renderer(Core& core) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    for (const auto& player : get_players()) {
        if (player.pointer == 0) continue;

        vec2_t<float> screen_position;
        if (core.m_projection_utils->WorldToScreenDX(
            player.position, screen_position, get_view_matrix())) {

            draw_list->AddCircleFilled(
                ImVec2(screen_position.x, screen_position.y),
                5.f, IM_COL32(255, 0, 0, 255), 8);
        }
    }
}

int main() {
    std::unique_ptr<Core> core = std::make_unique<Core>();

    core->with_target_type(TargetKind::Local)
        .with_logger_backend(LoggerBackend::Console)
        .with_logger_level(LogLevel::Debug)
        .with_target("AssaultCube", "ac_client.exe")
        .with_window_title("AssaultCube Cheat");

    if (!core->initialize()) {
        log_critical("Failed to initialize core");
        return 1;
    }

    core->m_update_manager->force_update("globals");
    core->m_update_manager->force_update("entity_scan");

    core->register_function(reader);
    core->register_function(renderer);

    while (core->update()) {}
    return 0;
}