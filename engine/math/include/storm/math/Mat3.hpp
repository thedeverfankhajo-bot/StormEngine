#pragma once

#include "Vec3.hpp"
#include <cmath>

namespace storm::math {

struct Mat3 {
    float m[3][3]{};

    constexpr Mat3() = default;

    constexpr Mat3(float diagonal) {
        m[0][0] = diagonal;
        m[1][1] = diagonal;
        m[2][2] = diagonal;
    }

    static constexpr Mat3 identity() { return Mat3(1.0f); }

    static Mat3 rotationZ(float radians) {
        Mat3 result(1.0f);
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result.m[0][0] = c; result.m[0][1] = -s;
        result.m[1][0] = s; result.m[1][1] = c;
        return result;
    }

    constexpr Vec3 operator*(const Vec3& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        };
    }

    constexpr Mat3 operator*(const Mat3& rhs) const {
        Mat3 result;
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                for (int k = 0; k < 3; ++k)
                    result.m[row][col] += m[row][k] * rhs.m[k][col];
        return result;
    }
};

} // namespace storm::math
