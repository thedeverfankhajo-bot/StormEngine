#pragma once
#include "storm/foundation/result.hpp"
#include <array>
#include <cstddef>
namespace Storm::Math {
struct Vector2 final { float x{0.0F}; float y{0.0F}; [[nodiscard]] bool is_finite() const noexcept; };
class Matrix3 final {
public:
 [[nodiscard]] static Matrix3 identity() noexcept;
 [[nodiscard]] float at(std::size_t row,std::size_t column) const;
 [[nodiscard]] bool is_finite() const noexcept;
 friend Matrix3 operator*(const Matrix3& lhs,const Matrix3& rhs) noexcept;
private:
 friend struct Transform2D;
 std::array<float,9U> values_{};
};
// +X right, +Y up, positive angles counter-clockwise; local composition T*R*S.
struct Transform2D final {
 Vector2 translation{};
 float rotation_radians{0.0F};
 Vector2 scale{1.0F,1.0F};
 [[nodiscard]] bool is_valid() const noexcept;
 [[nodiscard]] Result<Matrix3> to_matrix() const;
};
} // namespace Storm::Math
