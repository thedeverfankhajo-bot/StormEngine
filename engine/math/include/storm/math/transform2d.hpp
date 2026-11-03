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
 [[nodiscard]] static Result<Matrix3> multiply_checked(const Matrix3& lhs,const Matrix3& rhs);
 [[nodiscard]] Result<Vector2> transform_point(Vector2 point) const;
 friend Matrix3 operator*(const Matrix3& lhs,const Matrix3& rhs) noexcept;
private:
 friend struct Transform2D;
 friend struct Camera2D;
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

// Platform-neutral 2D camera. viewport_size uses logical viewport units;
// positive zoom increases visible-world magnification. Output is NDC [-1, 1].
struct Camera2D final {
 Vector2 position{};
 float rotation_radians{0.0F};
 Vector2 viewport_size{1.0F,1.0F};
 float zoom{1.0F};
 [[nodiscard]] bool is_valid() const noexcept;
 [[nodiscard]] Result<Matrix3> view_matrix() const;
 [[nodiscard]] Result<Matrix3> projection_matrix() const;
 [[nodiscard]] Result<Matrix3> view_projection() const;
 [[nodiscard]] Result<Vector2> world_to_ndc(Vector2 world_point) const;
};
} // namespace Storm::Math
