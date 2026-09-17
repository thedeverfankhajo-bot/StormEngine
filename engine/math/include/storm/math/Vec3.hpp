#pragma once

#include <cmath>

namespace storm::math {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Vec3() = default;
    constexpr Vec3(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}

    constexpr Vec3 operator+(const Vec3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    constexpr Vec3 operator-(const Vec3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    constexpr Vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    constexpr Vec3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    constexpr Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    constexpr Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    constexpr Vec3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

    constexpr float lengthSquared() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSquared()); }

    Vec3 normalized() const {
        const float len = length();
        return std::isfinite(len) && len > 0.0f ? *this / len : Vec3{};
    }

    constexpr float dot(const Vec3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

    constexpr Vec3 cross(const Vec3& rhs) const {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    static constexpr Vec3 zero() { return {0.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 one() { return {1.0f, 1.0f, 1.0f}; }
    static constexpr Vec3 up() { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Vec3 right() { return {1.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 forward() { return {0.0f, 0.0f, -1.0f}; }
};

constexpr Vec3 operator*(float scalar, const Vec3& value) { return value * scalar; }

} // namespace storm::math
