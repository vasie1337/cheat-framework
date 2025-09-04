#pragma once

struct CachedPlayer {
    uint32_t pointer = 0;
    vec3_t<float> position;
    int health = 0;
    int team = 0;

    bool data_cached = false;
};