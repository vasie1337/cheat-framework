#pragma once

struct CacheEntity
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

struct CachePlayer
{
    CacheEntity entity;
    uintptr_t controller_pawn = 0;
    uintptr_t list_pawn = 0;
    uintptr_t player_pawn = 0;
    uintptr_t bone_array = 0;
    SkeletonBone bones[64];
};