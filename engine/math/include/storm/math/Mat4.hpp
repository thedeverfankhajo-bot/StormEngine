#pragma once

#include "Vec3.hpp"
#include "Vec4.hpp"
#include <cmath>

namespace storm::math {

struct Mat4 {
    float m[4][4]{};

    constexpr Mat4() = default;
    constexpr Mat4(float diagonal) {
        m[0][0] = diagonal;
        m[1][1] = diagonal;
        m[2][2] = diagonal;
        m[3][3] = diagonal;
    }

    static constexpr Mat4 identity() { return Mat4(1.0f); }

    static constexpr Mat4 translation(const Vec3& t) {
        Mat4 result(1.0f);
        result.m[0][3] = t.x;
        result.m[1][3] = t.y;
        result.m[2][3] = t.z;
        return result;
    }

    static constexpr Mat4 scale(const Vec3& s) {
        Mat4 result(1.0f);
        result.m[0][0] = s.x;
        result.m[1][1] = s.y;
        result.m[2][2] = s.z;
        return result;
    }

    static Mat4 rotationX(float radians) {
        Mat4 result(1.0f);
        const float c = std::cos(radians), s = std::sin(radians);
        result.m[1][1] = c; result.m[1][2] = -s;
        result.m[2][1] = s; result.m[2][2] = c;
        return result;
    }

    static Mat4 rotationY(float radians) {
        Mat4 result(1.0f);
        const float c = std::cos(radians), s = std::sin(radians);
        result.m[0][0] = c; result.m[0][2] = s;
        result.m[2][0] = -s; result.m[2][2] = c;
        return result;
    }

    static Mat4 rotationZ(float radians) {
        Mat4 result(1.0f);
        const float c = std::cos(radians), s = std::sin(radians);
        result.m[0][0] = c; result.m[0][1] = -s;
        result.m[1][0] = s; result.m[1][1] = c;
        return result;
    }

    // Right-handed OpenGL-style projection, with NDC depth in [-1, 1].
    static Mat4 perspective(float verticalFovRadians, float aspect, float nearPlane, float farPlane) {
        Mat4 result;
        if (!(verticalFovRadians > 0.0f) || !(aspect > 0.0f) ||
            !(nearPlane > 0.0f) || !(farPlane > nearPlane)) return result;
        const float tanHalfFov = std::tan(verticalFovRadians * 0.5f);
        if (!(tanHalfFov > 0.0f) || !std::isfinite(tanHalfFov)) return result;
        result.m[0][0] = 1.0f / (aspect * tanHalfFov);
        result.m[1][1] = 1.0f / tanHalfFov;
        result.m[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[2][3] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result.m[3][2] = -1.0f;
        return result;
    }

    static Mat4 orthographic(float left, float right, float bottom, float top,
                             float nearPlane, float farPlane) {
        if (left == right || bottom == top || nearPlane == farPlane) return Mat4{};
        Mat4 result(1.0f);
        result.m[0][0] = 2.0f / (right - left);
        result.m[1][1] = 2.0f / (top - bottom);
        result.m[2][2] = -2.0f / (farPlane - nearPlane);
        result.m[0][3] = -(right + left) / (right - left);
        result.m[1][3] = -(top + bottom) / (top - bottom);
        result.m[2][3] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        return result;
    }

    constexpr Mat4 transposed() const {
        Mat4 result;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                result.m[row][col] = m[col][row];
        return result;
    }

    constexpr Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
            m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
        };
    }

    constexpr Mat4 operator*(const Mat4& rhs) const {
        Mat4 result;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                for (int k = 0; k < 4; ++k)
                    result.m[row][col] += m[row][k] * rhs.m[k][col];
        return result;
    }
};

} // namespace storm::math
