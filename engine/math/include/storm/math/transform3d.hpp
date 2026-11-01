#pragma once

#include "storm/foundation/result.hpp"

#include <array>
#include <cstddef>

namespace Storm::Math {

struct Vector3 final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};

    [[nodiscard]] bool is_finite() const noexcept;
};

struct Quaternion final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};

    [[nodiscard]] static Quaternion identity() noexcept;
    [[nodiscard]] bool is_finite() const noexcept;
    [[nodiscard]] Result<Quaternion> normalized() const;

    [[nodiscard]] static Result<Quaternion> from_axis_angle(
        Vector3 axis,
        float angle_radians);
};

// A 4x4 matrix using column-major storage and column-vector multiplication.
// at(row, column) is bounds-checked and throws std::out_of_range on invalid indices.
class Matrix4 final {
public:
    [[nodiscard]] static Matrix4 identity() noexcept;
    [[nodiscard]] float at(std::size_t row, std::size_t column) const;
    [[nodiscard]] bool is_finite() const noexcept;

    friend Matrix4 operator*(
        const Matrix4& lhs,
        const Matrix4& rhs) noexcept;

private:
    friend struct Transform3D;
    std::array<float, 16U> values_{};
};

struct Transform3D final {
    Vector3 translation{};
    Quaternion rotation{};
    Vector3 scale{1.0F, 1.0F, 1.0F};

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] Result<Matrix4> to_matrix() const;
};

} // namespace Storm::Math
