#pragma once

#include <cmath>

namespace storm::math {

struct Vec1 {
    float x{0.0f};
    constexpr Vec1() = default;
    constexpr explicit Vec1(float value) : x(value) {}
    constexpr Vec1 operator+(Vec1 rhs) const noexcept { return Vec1{x + rhs.x}; }
    constexpr Vec1 operator-(Vec1 rhs) const noexcept { return Vec1{x - rhs.x}; }
    constexpr Vec1 operator-() const noexcept { return Vec1{-x}; }
    constexpr Vec1 operator*(float scalar) const noexcept { return Vec1{x * scalar}; }
    constexpr Vec1 operator/(float scalar) const noexcept { return Vec1{x / scalar}; }
    constexpr Vec1& operator+=(Vec1 rhs) noexcept { x += rhs.x; return *this; }
    constexpr Vec1& operator-=(Vec1 rhs) noexcept { x -= rhs.x; return *this; }
    constexpr Vec1& operator*=(float scalar) noexcept { x *= scalar; return *this; }
    constexpr Vec1& operator/=(float scalar) noexcept { x /= scalar; return *this; }
    constexpr float lengthSquared() const noexcept { return x * x; }
    float length() const noexcept { return std::fabs(x); }
    Vec1 normalized() const noexcept {
        return x == 0.0f || !std::isfinite(x) ? Vec1{} : Vec1{x > 0.0f ? 1.0f : -1.0f};
    }
    constexpr float dot(Vec1 rhs) const noexcept { return x * rhs.x; }
};

constexpr Vec1 operator*(float scalar, Vec1 value) noexcept { return value * scalar; }

} // namespace storm::math
