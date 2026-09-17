#pragma once

#include "Vec3.hpp"
#include "Mat4.hpp"
#include <algorithm>
#include <cmath>

namespace storm::math {

struct Quaternion {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float w{1.0f};

    constexpr Quaternion() = default;
    constexpr Quaternion(float xValue, float yValue, float zValue, float wValue)
        : x(xValue), y(yValue), z(zValue), w(wValue) {}

    static Quaternion identity() { return {}; }

    static Quaternion fromAxisAngle(const Vec3& axis, float radians) {
        if (!std::isfinite(radians)) return {};
        const float axisLength = axis.length();
        if (!(axisLength > 0.0f) || !std::isfinite(axisLength)) return {};
        const Vec3 n = axis / axisLength;
        const float half = radians * 0.5f;
        const float s = std::sin(half);
        return {n.x * s, n.y * s, n.z * s, std::cos(half)};
    }

    static Quaternion fromEuler(float pitch, float yaw, float roll) {
        if (!std::isfinite(pitch) || !std::isfinite(yaw) || !std::isfinite(roll)) return {};
        const float hp = pitch * 0.5f;
        const float hy = yaw * 0.5f;
        const float hr = roll * 0.5f;
        const float cp = std::cos(hp), sp = std::sin(hp);
        const float cy = std::cos(hy), sy = std::sin(hy);
        const float cr = std::cos(hr), sr = std::sin(hr);
        return {
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        };
    }

    constexpr Quaternion operator*(const Quaternion& q) const {
        return {
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w,
            w*q.w - x*q.x - y*q.y - z*q.z
        };
    }

    constexpr Quaternion conjugate() const { return {-x, -y, -z, w}; }
    constexpr float lengthSquared() const { return x*x + y*y + z*z + w*w; }

    Quaternion normalized() const {
        const float lenSq = lengthSquared();
        if (!(lenSq > 0.0f) || !std::isfinite(lenSq)) return {};
        const float invLen = 1.0f / std::sqrt(lenSq);
        return {x*invLen, y*invLen, z*invLen, w*invLen};
    }

    Quaternion inverse() const {
        const float lenSq = lengthSquared();
        if (!(lenSq > 0.0f) || !std::isfinite(lenSq)) return {};
        return {-x/lenSq, -y/lenSq, -z/lenSq, w/lenSq};
    }

    Vec3 rotate(const Vec3& v) const {
        const Quaternion q = normalized();
        const Vec3 u{q.x, q.y, q.z};
        const float s = q.w;
        return u * (2.0f * u.dot(v)) + v * (s*s - u.dot(u)) + u.cross(v) * (2.0f * s);
    }

    Mat4 toMat4() const {
        const Quaternion q = normalized();
        const float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
        const float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
        const float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
        Mat4 result(1.0f);
        result.m[0][0] = 1.0f - 2.0f*(yy + zz);
        result.m[0][1] = 2.0f*(xy - wz);
        result.m[0][2] = 2.0f*(xz + wy);
        result.m[1][0] = 2.0f*(xy + wz);
        result.m[1][1] = 1.0f - 2.0f*(xx + zz);
        result.m[1][2] = 2.0f*(yz - wx);
        result.m[2][0] = 2.0f*(xz - wy);
        result.m[2][1] = 2.0f*(yz + wx);
        result.m[2][2] = 1.0f - 2.0f*(xx + yy);
        return result;
    }

    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        if (!std::isfinite(t)) return a.normalized();
        Quaternion q1 = a.normalized();
        Quaternion q2 = b.normalized();
        float dot = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
        if (!std::isfinite(dot)) return {};
        if (dot < 0.0f) { q2 = {-q2.x, -q2.y, -q2.z, -q2.w}; dot = -dot; }
        dot = std::clamp(dot, -1.0f, 1.0f);
        if (dot > 0.9995f) {
            return Quaternion{
                q1.x + t*(q2.x-q1.x), q1.y + t*(q2.y-q1.y),
                q1.z + t*(q2.z-q1.z), q1.w + t*(q2.w-q1.w)
            }.normalized();
        }
        const float theta = std::acos(dot);
        const float sinTheta = std::sin(theta);
        if (!(sinTheta > 1e-6f)) return q1;
        const float aWeight = std::sin((1.0f-t)*theta) / sinTheta;
        const float bWeight = std::sin(t*theta) / sinTheta;
        return Quaternion{
            q1.x*aWeight + q2.x*bWeight, q1.y*aWeight + q2.y*bWeight,
            q1.z*aWeight + q2.z*bWeight, q1.w*aWeight + q2.w*bWeight
        }.normalized();
    }
};

} // namespace storm::math
