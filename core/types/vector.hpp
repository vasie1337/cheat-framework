#pragma once

#include <cmath>
#include <imgui.h>

template <typename T>
class Vector2
{
public:
    T x, y;

    Vector2() : x(0), y(0) {}
    Vector2(T x, T y) : x(x), y(y) {}

    Vector2(const ImVec2 &other) : x(other.x), y(other.y) {}
    Vector2(const Vector2 &other) : x(other.x), y(other.y) {}
    Vector2(const Vector2 &&other) : x(other.x), y(other.y) {}
    Vector2(const ImVec2 &&other) : x(other.x), y(other.y) {}

    Vector2 &operator=(const ImVec2 &other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    Vector2 &operator=(const Vector2 &other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    Vector2 &operator=(const Vector2 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    Vector2 &operator=(const ImVec2 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        return *this;
    }

    Vector2 operator+(const Vector2 &other) const noexcept { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2 &other) const noexcept { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(const Vector2 &other) const noexcept { return Vector2(x * other.x, y * other.y); }
    Vector2 operator/(const Vector2 &other) const noexcept { return Vector2(x / other.x, y / other.y); }
    Vector2 operator*(T scalar) const noexcept { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(T scalar) const noexcept { return Vector2(x / scalar, y / scalar); }
    bool operator==(const Vector2 &other) const noexcept { return x == other.x && y == other.y; }
    bool operator!=(const Vector2 &other) const noexcept { return x != other.x || y != other.y; }
    T length() const noexcept { return std::sqrt(x * x + y * y); }
    T length_squared() const noexcept { return x * x + y * y; }
    T dot(const Vector2 &other) const noexcept { return x * other.x + y * other.y; }
    T cross(const Vector2 &other) const noexcept { return x * other.y - y * other.x; }
    Vector2 normalize() const noexcept { return *this / length(); }
};

template <typename T>
class Vector3
{
public:
    T x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x, T y, T z) : x(x), y(y), z(z) {}

    Vector3(const Vector3 &other) : x(other.x), y(other.y), z(other.z) {}
    Vector3(const Vector3 &&other) : x(other.x), y(other.y), z(other.z) {}

    Vector3 &operator=(const Vector3 &other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }
    Vector3 &operator=(const Vector3 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    Vector3 operator+(const Vector3 &other) const noexcept { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3 &other) const noexcept { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(const Vector3 &other) const noexcept { return Vector3(x * other.x, y * other.y, z * other.z); }
    Vector3 operator/(const Vector3 &other) const noexcept { return Vector3(x / other.x, y / other.y, z / other.z); }
    Vector3 operator*(T scalar) const noexcept { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 operator/(T scalar) const noexcept { return Vector3(x / scalar, y / scalar, z / scalar); }
    bool operator==(const Vector3 &other) const noexcept { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vector3 &other) const noexcept { return x != other.x || y != other.y || z != other.z; }
    T length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
    T length_squared() const noexcept { return x * x + y * y + z * z; }
    T dot(const Vector3 &other) const noexcept { return x * other.x + y * other.y + z * other.z; }
    Vector3 cross(const Vector3 &other) const noexcept { return Vector3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x); }
    Vector3 normalize() const noexcept { return *this / length(); }
};

template <typename T>
class Vector4
{
public:
    T x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    Vector4(const Vector4 &other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    Vector4(const Vector4 &&other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    Vector4(const ImVec4 &other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    Vector4(const ImVec4 &&other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

    Vector4 &operator=(const ImVec4 &other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
    Vector4 &operator=(const Vector4 &other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
    Vector4 &operator=(const Vector4 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
    Vector4 &operator=(const ImVec4 &&other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }

    Vector4 operator+(const Vector4 &other) const noexcept { return Vector4(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vector4 operator-(const Vector4 &other) const noexcept { return Vector4(x - other.x, y - other.y, z - other.z, w - other.w); }
    Vector4 operator*(const Vector4 &other) const noexcept { return Vector4(x * other.x, y * other.y, z * other.z, w * other.w); }
    Vector4 operator/(const Vector4 &other) const noexcept { return Vector4(x / other.x, y / other.y, z / other.z, w / other.w); }
    Vector4 operator*(T scalar) const noexcept { return Vector4(x * scalar, y * scalar, z * scalar, w * scalar); }
    Vector4 operator/(T scalar) const noexcept { return Vector4(x / scalar, y / scalar, z / scalar, w / scalar); }
    bool operator==(const Vector4 &other) const noexcept { return x == other.x && y == other.y && z == other.z && w == other.w; }
    bool operator!=(const Vector4 &other) const noexcept { return x != other.x || y != other.y || z != other.z || w != other.w; }
    T length() const noexcept { return std::sqrt(x * x + y * y + z * z + w * w); }
    T length_squared() const noexcept { return x * x + y * y + z * z + w * w; }
    T dot(const Vector4 &other) const noexcept { return x * other.x + y * other.y + z * other.z + w * other.w; }
    Vector4 normalize() const noexcept { return *this / length(); }
};
