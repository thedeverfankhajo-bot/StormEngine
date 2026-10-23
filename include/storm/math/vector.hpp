#pragma once

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <optional>

namespace storm::math {

// Small, dimension-safe floating-point vectors for common engine-side math.
// Larger or SIMD-specialized math types can be introduced behind this API
// after profiling establishes their requirements.
template <std::size_t Dimensions>
    requires(Dimensions >= 1 && Dimensions <= 3)
class Vector final {
public:
    static constexpr std::size_t dimensions = Dimensions;

    constexpr Vector() noexcept = default;

    template <typename... Components>
        requires(sizeof...(Components) == Dimensions &&
                 (std::convertible_to<Components, float> && ...))
    constexpr explicit Vector(Components... components) noexcept
        : components_{static_cast<float>(components)...} {}

    [[nodiscard]] constexpr float& operator[](std::size_t index) noexcept {
        return components_[index];
    }

    [[nodiscard]] constexpr float operator[](std::size_t index) const noexcept {
        return components_[index];
    }

    [[nodiscard]] constexpr const std::array<float, Dimensions>& values() const noexcept {
        return components_;
    }

    [[nodiscard]] constexpr Vector operator+(const Vector& other) const noexcept {
        Vector result;
        for (std::size_t index = 0; index < Dimensions; ++index) {
            result[index] = components_[index] + other[index];
        }
        return result;
    }

    [[nodiscard]] constexpr Vector operator-(const Vector& other) const noexcept {
        Vector result;
        for (std::size_t index = 0; index < Dimensions; ++index) {
            result[index] = components_[index] - other[index];
        }
        return result;
    }

    [[nodiscard]] constexpr Vector operator*(float scalar) const noexcept {
        Vector result;
        for (std::size_t index = 0; index < Dimensions; ++index) {
            result[index] = components_[index] * scalar;
        }
        return result;
    }

    constexpr Vector& operator+=(const Vector& other) noexcept {
        for (std::size_t index = 0; index < Dimensions; ++index) {
            components_[index] += other[index];
        }
        return *this;
    }

    constexpr Vector& operator-=(const Vector& other) noexcept {
        for (std::size_t index = 0; index < Dimensions; ++index) {
            components_[index] -= other[index];
        }
        return *this;
    }

    [[nodiscard]] constexpr float dot(const Vector& other) const noexcept {
        float result = 0.0F;
        for (std::size_t index = 0; index < Dimensions; ++index) {
            result += components_[index] * other[index];
        }
        return result;
    }

    [[nodiscard]] constexpr float length_squared() const noexcept {
        return dot(*this);
    }

    [[nodiscard]] float length() const noexcept {
        return std::sqrt(length_squared());
    }

    [[nodiscard]] bool is_finite() const noexcept {
        for (const float component : components_) {
            if (!std::isfinite(component)) {
                return false;
            }
        }
        return true;
    }

    // Returns no value for non-finite input or a vector too close to zero.
    // Callers must handle that result rather than silently using an invalid direction.
    [[nodiscard]] std::optional<Vector> normalized(float epsilon = 1.0e-6F) const noexcept {
        if (!is_finite() || !std::isfinite(epsilon) || epsilon < 0.0F) {
            return std::nullopt;
        }

        const float squared_length = length_squared();
        if (!std::isfinite(squared_length) || squared_length <= epsilon * epsilon) {
            return std::nullopt;
        }

        const float magnitude = std::sqrt(squared_length);
        if (!std::isfinite(magnitude) || magnitude <= epsilon) {
            return std::nullopt;
        }

        Vector result;
        for (std::size_t index = 0; index < Dimensions; ++index) {
            result[index] = components_[index] / magnitude;
        }
        return result;
    }

    [[nodiscard]] constexpr bool operator==(const Vector&) const noexcept = default;

private:
    std::array<float, Dimensions> components_{};
};

template <std::size_t Dimensions>
    requires(Dimensions >= 1 && Dimensions <= 3)
[[nodiscard]] constexpr Vector<Dimensions> operator*(float scalar,
                                                       const Vector<Dimensions>& vector) noexcept {
    return vector * scalar;
}

using Vec1 = Vector<1>;
using Vec2 = Vector<2>;
using Vec3 = Vector<3>;

} // namespace storm::math
