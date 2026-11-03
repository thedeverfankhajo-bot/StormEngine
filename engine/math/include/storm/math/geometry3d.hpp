#pragma once

#include "storm/math/transform3d.hpp"

#include <optional>

namespace Storm::Math {

// Axis-aligned bounds in the engine's +X-right, +Y-up, +Z-forward world space.
// Degenerate bounds are valid; minimum must not exceed maximum on any axis.
struct Aabb3D final {
    Vector3 minimum{};
    Vector3 maximum{};

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] bool contains(Vector3 point) const noexcept;
    [[nodiscard]] bool intersects(const Aabb3D& other) const noexcept;

    [[nodiscard]] static Result<Aabb3D> from_center_half_extents(
        Vector3 center,
        Vector3 half_extents);

    [[nodiscard]] Result<Aabb3D> merged(const Aabb3D& other) const;
};

// A sphere with a finite, non-negative radius. A zero radius is a point sphere.
struct Sphere3D final {
    Vector3 center{};
    float radius{0.0F};

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] bool contains(Vector3 point) const noexcept;
};

// Constructed rays always store a normalized direction, so hit distances are
// expressed in world units independently of the input direction's magnitude.
class Ray3D final {
public:
    [[nodiscard]] static Result<Ray3D> from_direction(
        Vector3 origin,
        Vector3 direction);

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] Vector3 origin() const noexcept;
    [[nodiscard]] Vector3 direction() const noexcept;

private:
    Ray3D(Vector3 origin, Vector3 normalized_direction) noexcept;

    Vector3 origin_{};
    Vector3 direction_{0.0F, 0.0F, 1.0F};
};

// Plane equation: dot(normal(), point) + offset() == 0.
// Factory-created planes always have a normalized, finite normal.
class Plane3D final {
public:
    [[nodiscard]] static Result<Plane3D> from_point_normal(
        Vector3 point,
        Vector3 normal);

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] Vector3 normal() const noexcept;
    [[nodiscard]] float offset() const noexcept;
    [[nodiscard]] Result<double> signed_distance(Vector3 point) const;

private:
    Plane3D(Vector3 normalized_normal, float offset) noexcept;

    Vector3 normal_{0.0F, 1.0F, 0.0F};
    float offset_{0.0F};
};

struct RayHit3D final {
    double distance{0.0};
    Vector3 point{};
    // Unit surface normal; zero when the ray starts strictly inside a volume.
    Vector3 normal{};
};

// max_distance is finite, non-negative, and inclusive. Invalid shapes or query
// values return InvalidArgument; a valid query with no hit returns nullopt.
[[nodiscard]] Result<std::optional<RayHit3D>> intersect_ray_aabb(
    const Ray3D& ray,
    const Aabb3D& bounds,
    double max_distance);

[[nodiscard]] Result<std::optional<RayHit3D>> intersect_ray_sphere(
    const Ray3D& ray,
    const Sphere3D& sphere,
    double max_distance);

// Parallel/coplanar rays have no unique plane hit and return nullopt.
[[nodiscard]] Result<std::optional<RayHit3D>> intersect_ray_plane(
    const Ray3D& ray,
    const Plane3D& plane,
    double max_distance);

} // namespace Storm::Math
