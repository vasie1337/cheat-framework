#pragma once
#include "../constants/constants.hpp"

struct SkeletonBone
{
    vec3_t<float> position;
    float scale;
    vec4_t<float> rotation;
};

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