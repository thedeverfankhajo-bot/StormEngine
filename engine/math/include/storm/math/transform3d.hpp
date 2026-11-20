#pragma once

#include "storm/foundation/result.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Storm::Math {

struct Vector3 final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};

    [[nodiscard]] bool is_finite() const noexcept;
    [[nodiscard]] double length_squared() const noexcept;
    [[nodiscard]] double length() const noexcept;
    [[nodiscard]] double dot(Vector3 other) const noexcept;
    [[nodiscard]] Result<Vector3> cross(Vector3 other) const;
    [[nodiscard]] Result<Vector3> normalized() const;
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
// Camera constructors use a right-handed view space looking down -Z and a [0, 1]
// normalized device depth interval (near -> 0, far -> 1).
class Matrix4 final {
public:
    [[nodiscard]] static Matrix4 identity() noexcept;

    // Validates both operands and rejects non-finite or out-of-float-range products.
    [[nodiscard]] static Result<Matrix4> multiply_checked(
        const Matrix4& lhs,
        const Matrix4& rhs);
    [[nodiscard]] float at(std::size_t row, std::size_t column) const;
    [[nodiscard]] bool is_finite() const noexcept;

    [[nodiscard]] static Result<Matrix4> perspective_rh_zo(
        float vertical_fov_radians,
        float aspect_ratio,
        float near_plane,
        float far_plane);

    [[nodiscard]] static Result<Matrix4> orthographic_rh_zo(
        float left,
        float right,
        float bottom,
        float top,
        float near_plane,
        float far_plane);

    [[nodiscard]] static Result<Matrix4> look_at_rh(
        Vector3 eye,
        Vector3 target,
        Vector3 up);

    [[nodiscard]] Result<Vector3> transform_point(Vector3 point) const;
    [[nodiscard]] Result<Vector3> transform_direction(Vector3 direction) const;

    // Unchecked convenience operator; the result may be non-finite on overflow.
    // Use multiply_checked() when errors must be propagated.
    friend Matrix4 operator*(
        const Matrix4& lhs,
        const Matrix4& rhs) noexcept;

private:
    friend struct Transform3D;
    std::array<float, 16U> values_{};
};


// A platform-neutral 3D camera. View space is right-handed and looks down -Z;
// perspective and orthographic depth both use the zero-to-one [near, far] range.
// position/target/up describe the camera pose in world space. For perspective
// cameras, aspect_ratio is the logical viewport width divided by its height.
enum class Camera3DProjection : std::uint8_t {
    Perspective,
    Orthographic,
};

struct Camera3D final {
    Vector3 position{};
    Vector3 target{0.0F, 0.0F, -1.0F};
    Vector3 up{0.0F, 1.0F, 0.0F};
    Camera3DProjection projection{Camera3DProjection::Perspective};

    float vertical_fov_radians{1.0471975512F};
    float aspect_ratio{1.0F};
    float near_plane{0.1F};
    float far_plane{1000.0F};

    // Used only when projection == Orthographic.
    float left{-1.0F};
    float right{1.0F};
    float bottom{-1.0F};
    float top{1.0F};

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] Result<Matrix4> view_matrix() const;
    [[nodiscard]] Result<Matrix4> projection_matrix() const;
    [[nodiscard]] Result<Matrix4> view_projection_matrix() const;
    [[nodiscard]] Result<Vector3> world_to_ndc(Vector3 world_position) const;
};

struct Transform3D final {
    Vector3 translation{};
    Quaternion rotation{};
    Vector3 scale{1.0F, 1.0F, 1.0F};

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] Result<Matrix4> to_matrix() const;
};

} // namespace Storm::Math
