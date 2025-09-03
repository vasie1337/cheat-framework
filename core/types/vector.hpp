#pragma once

#include <cmath>
#include <imgui.h>

template <typename T>
class vec2_t
{
public:
    T x, y;

    vec2_t() : x(0), y(0) {}
    vec2_t(T x, T y) : x(x), y(y) {}

    vec2_t(const ImVec2 &other) : x(other.x), y(other.y) {}
    vec2_t(const vec2_t &other) : x(other.x), y(other.y) {}
    vec2_t(const vec2_t &&other) : x(other.x), y(other.y) {}
    vec2_t(const ImVec2 &&other) : x(other.x), y(other.y) {}

    vec2_t &operator=(const ImVec2 &other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    vec2_t &operator=(const vec2_t &other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    vec2_t &operator=(const vec2_t &&other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    vec2_t &operator=(const ImVec2 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }

    vec2_t operator+(const vec2_t &other) const noexcept { return vec2_t(x + other.x, y + other.y); }
    vec2_t operator-(const vec2_t &other) const noexcept { return vec2_t(x - other.x, y - other.y); }
    vec2_t operator*(const vec2_t &other) const noexcept { return vec2_t(x * other.x, y * other.y); }
    vec2_t operator/(const vec2_t &other) const noexcept { return vec2_t(x / other.x, y / other.y); }
    vec2_t operator*(T scalar) const noexcept { return vec2_t(x * scalar, y * scalar); }
    vec2_t operator/(T scalar) const noexcept { return vec2_t(x / scalar, y / scalar); }
    bool operator==(const vec2_t &other) const noexcept { return x == other.x && y == other.y; }
    bool operator!=(const vec2_t &other) const noexcept { return x != other.x || y != other.y; }
    T length() const noexcept { return std::sqrt(x * x + y * y); }
    T length_squared() const noexcept { return x * x + y * y; }
    T dot(const vec2_t &other) const noexcept { return x * other.x + y * other.y; }
    T cross(const vec2_t &other) const noexcept { return x * other.y - y * other.x; }
    vec2_t normalize() const noexcept { return *this / length(); }
};

template <typename T>
class vec3_t
{
public:
    T x, y, z;

    vec3_t() : x(0), y(0), z(0) {}
    vec3_t(T x, T y, T z) : x(x), y(y), z(z) {}

    vec3_t(const vec3_t &other) : x(other.x), y(other.y), z(other.z) {}
    vec3_t(const vec3_t &&other) : x(other.x), y(other.y), z(other.z) {}

    vec3_t &operator=(const vec3_t &other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }
    vec3_t &operator=(const vec3_t &&other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    vec3_t operator+(const vec3_t &other) const noexcept { return vec3_t(x + other.x, y + other.y, z + other.z); }
    vec3_t operator-(const vec3_t &other) const noexcept { return vec3_t(x - other.x, y - other.y, z - other.z); }
    vec3_t operator*(const vec3_t &other) const noexcept { return vec3_t(x * other.x, y * other.y, z * other.z); }
    vec3_t operator/(const vec3_t &other) const noexcept { return vec3_t(x / other.x, y / other.y, z / other.z); }
    vec3_t operator*(T scalar) const noexcept { return vec3_t(x * scalar, y * scalar, z * scalar); }
    vec3_t operator/(T scalar) const noexcept { return vec3_t(x / scalar, y / scalar, z / scalar); }
    bool operator==(const vec3_t &other) const noexcept { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const vec3_t &other) const noexcept { return x != other.x || y != other.y || z != other.z; }
    T length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
    T length_squared() const noexcept { return x * x + y * y + z * z; }
    T dot(const vec3_t &other) const noexcept { return x * other.x + y * other.y + z * other.z; }
    vec3_t cross(const vec3_t &other) const noexcept { return vec3_t(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x); }
    vec3_t normalize() const noexcept { return *this / length(); }
};

template <typename T>
class vec4_t
{
public:
    T x, y, z, w;

    vec4_t() : x(0), y(0), z(0), w(0) {}
    vec4_t(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    vec4_t(const vec4_t &other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    vec4_t(const vec4_t &&other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    vec4_t(const ImVec4 &other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    vec4_t(const ImVec4 &&other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

    vec4_t &operator=(const ImVec4 &other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
    vec4_t &operator=(const vec4_t &other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
    vec4_t &operator=(const vec4_t &&other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
    vec4_t &operator=(const ImVec4 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }

    vec4_t operator+(const vec4_t &other) const noexcept { return vec4_t(x + other.x, y + other.y, z + other.z, w + other.w); }
    vec4_t operator-(const vec4_t &other) const noexcept { return vec4_t(x - other.x, y - other.y, z - other.z, w - other.w); }
    vec4_t operator*(const vec4_t &other) const noexcept { return vec4_t(x * other.x, y * other.y, z * other.z, w * other.w); }
    vec4_t operator/(const vec4_t &other) const noexcept { return vec4_t(x / other.x, y / other.y, z / other.z, w / other.w); }
    vec4_t operator*(T scalar) const noexcept { return vec4_t(x * scalar, y * scalar, z * scalar, w * scalar); }
    vec4_t operator/(T scalar) const noexcept { return vec4_t(x / scalar, y / scalar, z / scalar, w / scalar); }
    bool operator==(const vec4_t &other) const noexcept { return x == other.x && y == other.y && z == other.z && w == other.w; }
    bool operator!=(const vec4_t &other) const noexcept { return x != other.x || y != other.y || z != other.z || w != other.w; }
    T length() const noexcept { return std::sqrt(x * x + y * y + z * z + w * w); }
    T length_squared() const noexcept { return x * x + y * y + z * z + w * w; }
    T dot(const vec4_t &other) const noexcept { return x * other.x + y * other.y + z * other.z + w * other.w; }
    vec4_t normalize() const noexcept { return *this / length(); }
};
