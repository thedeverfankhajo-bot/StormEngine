#pragma once

#include "storm/math/Vec2.hpp"
#include "storm/math/Vec3.hpp"
#include "storm/math/Vec4.hpp"

namespace storm::math {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kEpsilon = 1e-6f;

constexpr float radians(float degrees) {
    return degrees * (kPi / 180.0f);
}

constexpr float degrees(float radiansValue) {
    return radiansValue * (180.0f / kPi);
}

template <typename T>
constexpr T clamp(T value, T minValue, T maxValue) {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

template <typename T>
constexpr T lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

} // namespace storm::math
