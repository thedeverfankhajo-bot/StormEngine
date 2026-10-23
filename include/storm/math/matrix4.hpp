#pragma once

#include "storm/math/vector.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace storm::math {

// A compact 4x4 matrix using column-major storage and column-vector mathematics.
// Coordinate, rotation, composition and homogeneous-coordinate conventions are documented in
// docs/decisions/0002-coordinate-system-and-matrix-conventions.md.
class Matrix4 final {
public:
    static constexpr std::size_t dimension = 4U;
    static constexpr float minimum_homogeneous_w = 1.0e-6F;

    constexpr Matrix4() noexcept = default;

    [[nodiscard]] static constexpr Matrix4 identity() noexcept {
        Matrix4 result;
        for (std::size_t axis = 0U; axis < dimension; ++axis) {
            result(axis, axis) = 1.0F;
        }
        return result;
    }

    [[nodiscard]] static constexpr Matrix4 translation(const Vec3& offset) noexcept {
        Matrix4 result = identity();
        result(0U, 3U) = offset[0U];
        result(1U, 3U) = offset[1U];
        result(2U, 3U) = offset[2U];
        return result;
    }

    [[nodiscard]] static constexpr Matrix4 scale(const Vec3& factors) noexcept {
        Matrix4 result = identity();
        result(0U, 0U) = factors[0U];
        result(1U, 1U) = factors[1U];
        result(2U, 2U) = factors[2U];
        return result;
    }

    // Positive angles follow the right-hand rule; all angles are in radians.
    [[nodiscard]] static Matrix4 rotation_x(float radians) noexcept {
        Matrix4 result = identity();
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        result(1U, 1U) = cosine;
        result(1U, 2U) = -sine;
        result(2U, 1U) = sine;
        result(2U, 2U) = cosine;
        return result;
    }

    [[nodiscard]] static Matrix4 rotation_y(float radians) noexcept {
        Matrix4 result = identity();
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        result(0U, 0U) = cosine;
        result(0U, 2U) = sine;
        result(2U, 0U) = -sine;
        result(2U, 2U) = cosine;
        return result;
    }

    [[nodiscard]] static Matrix4 rotation_z(float radians) noexcept {
        Matrix4 result = identity();
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        result(0U, 0U) = cosine;
        result(0U, 1U) = -sine;
        result(1U, 0U) = sine;
        result(1U, 1U) = cosine;
        return result;
    }

    // Preconditions: row and column must be less than dimension.
    // Data is packed by columns: values()[column * 4 + row].
    [[nodiscard]] constexpr float& operator()(std::size_t row, std::size_t column) noexcept {
        return values_[column * dimension + row];
    }

    [[nodiscard]] constexpr float operator()(std::size_t row,
                                              std::size_t column) const noexcept {
        return values_[column * dimension + row];
    }

    [[nodiscard]] constexpr const std::array<float, dimension * dimension>& values() const noexcept {
        return values_;
    }

    [[nodiscard]] constexpr Matrix4 operator*(const Matrix4& other) const noexcept {
        Matrix4 result;
        for (std::size_t row = 0U; row < dimension; ++row) {
            for (std::size_t column = 0U; column < dimension; ++column) {
                for (std::size_t inner = 0U; inner < dimension; ++inner) {
                    result(row, column) += (*this)(row, inner) * other(inner, column);
                }
            }
        }
        return result;
    }

    [[nodiscard]] constexpr Matrix4 transposed() const noexcept {
        Matrix4 result;
        for (std::size_t row = 0U; row < dimension; ++row) {
            for (std::size_t column = 0U; column < dimension; ++column) {
                result(row, column) = (*this)(column, row);
            }
        }
        return result;
    }

    [[nodiscard]] bool is_finite() const noexcept {
        for (const float value : values_) {
            if (!std::isfinite(value)) {
                return false;
            }
        }
        return true;
    }

    // Applies the full homogeneous transform and perspective-divides by W.
    // Returns no value for invalid input, non-finite results, or near-zero W.
    [[nodiscard]] std::optional<Vec3> transform_point(const Vec3& point) const noexcept {
        if (!is_finite() || !point.is_finite()) {
            return std::nullopt;
        }

        const std::array<float, dimension> input{point[0U], point[1U], point[2U], 1.0F};
        std::array<float, dimension> transformed{};
        for (std::size_t row = 0U; row < dimension; ++row) {
            for (std::size_t column = 0U; column < dimension; ++column) {
                transformed[row] += (*this)(row, column) * input[column];
            }
        }

        const float homogeneous_w = transformed[3U];
        if (!std::isfinite(homogeneous_w) ||
            std::abs(homogeneous_w) <= minimum_homogeneous_w) {
            return std::nullopt;
        }

        const Vec3 result{transformed[0U] / homogeneous_w,
                          transformed[1U] / homogeneous_w,
                          transformed[2U] / homogeneous_w};
        return result.is_finite() ? std::optional<Vec3>{result} : std::nullopt;
    }

    // Applies only the upper-left 3x3 part; translation and perspective division are ignored.
    [[nodiscard]] std::optional<Vec3> transform_direction(const Vec3& direction) const noexcept {
        if (!is_finite() || !direction.is_finite()) {
            return std::nullopt;
        }

        Vec3 result;
        for (std::size_t row = 0U; row < 3U; ++row) {
            for (std::size_t column = 0U; column < 3U; ++column) {
                result[row] += (*this)(row, column) * direction[column];
            }
        }
        return result.is_finite() ? std::optional<Vec3>{result} : std::nullopt;
    }

    [[nodiscard]] constexpr bool operator==(const Matrix4&) const noexcept = default;

private:
    std::array<float, dimension * dimension> values_{};
};

using Mat4 = Matrix4;

} // namespace storm::math
