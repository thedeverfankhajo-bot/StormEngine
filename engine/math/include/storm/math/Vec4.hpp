#pragma once

#include <cmath>

namespace storm::math {

struct Vec4 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float w{0.0f};

    constexpr Vec4() = default;
    constexpr Vec4(float xValue, float yValue, float zValue, float wValue)
        : x(xValue), y(yValue), z(zValue), w(wValue) {}

    constexpr Vec4 operator+(const Vec4& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
    constexpr Vec4 operator-(const Vec4& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
    constexpr Vec4 operator-() const { return {-x, -y, -z, -w}; }
    constexpr Vec4 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar, w * scalar}; }
    constexpr Vec4 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar, w / scalar}; }

    constexpr Vec4& operator+=(const Vec4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
    constexpr Vec4& operator-=(const Vec4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
    constexpr Vec4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    constexpr Vec4& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    constexpr float lengthSquared() const { return x*x + y*y + z*z + w*w; }
    float length() const { return std::sqrt(lengthSquared()); }
    constexpr float dot(const Vec4& rhs) const { return x*rhs.x + y*rhs.y + z*rhs.z + w*rhs.w; }

    Vec4 normalized() const {
        const float len = length();
        return std::isfinite(len) && len > 0.0f ? *this / len : Vec4{};
    }
};

constexpr Vec4 operator*(float scalar, const Vec4& value) { return value * scalar; }

} // namespace storm::math
