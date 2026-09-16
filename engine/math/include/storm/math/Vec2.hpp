#pragma once

#include <cmath>

namespace storm::math {

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    constexpr Vec2() = default;
    constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

    constexpr Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

    constexpr Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    constexpr Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    constexpr Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

    constexpr float lengthSquared() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSquared()); }

    Vec2 normalized() const {
        const float len = length();
        return len > 0.0f ? *this / len : Vec2{};
    }

    constexpr float dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }
};

constexpr Vec2 operator*(float scalar, const Vec2& value) { return value * scalar; }

} // namespace storm::math
