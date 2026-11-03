#include "storm/math/geometry3d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Storm::Math {
namespace {

constexpr double kMinimumMagnitudeSquared = 1.0e-24;
constexpr double kUnitLengthSquaredTolerance = 1.0e-4;
constexpr double kParallelDirectionTolerance = 1.0e-12;

struct DoubleVector3 final {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

[[nodiscard]] Error invalid_geometry_input(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

[[nodiscard]] DoubleVector3 to_double(Vector3 value) noexcept {
    return DoubleVector3{
        static_cast<double>(value.x),
        static_cast<double>(value.y),
        static_cast<double>(value.z),
    };
}

[[nodiscard]] double dot(DoubleVector3 lhs, DoubleVector3 rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

[[nodiscard]] bool is_finite(DoubleVector3 value) noexcept {
    return std::isfinite(value.x) &&
        std::isfinite(value.y) &&
        std::isfinite(value.z);
}

[[nodiscard]] bool is_representable_float(double value) noexcept {
    constexpr double maximum =
        static_cast<double>(std::numeric_limits<float>::max());
    return std::isfinite(value) && value <= maximum && value >= -maximum;
}

[[nodiscard]] Result<Vector3> narrow_vector(
    DoubleVector3 value,
    const char* error_message) {
    if (!is_finite(value) ||
        !is_representable_float(value.x) ||
        !is_representable_float(value.y) ||
        !is_representable_float(value.z)) {
        return Result<Vector3>::failure(invalid_geometry_input(error_message));
    }

    return Result<Vector3>::success(Vector3{
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z),
    });
}

[[nodiscard]] Result<RayHit3D> make_hit(
    const Ray3D& ray,
    double distance,
    DoubleVector3 normal) {
    if (!std::isfinite(distance) || distance < 0.0 || !is_finite(normal)) {
        return Result<RayHit3D>::failure(
            invalid_geometry_input("Ray intersection produced a non-finite hit"));
    }

    const DoubleVector3 origin = to_double(ray.origin());
    const DoubleVector3 direction = to_double(ray.direction());
    const DoubleVector3 point{
        origin.x + (direction.x * distance),
        origin.y + (direction.y * distance),
        origin.z + (direction.z * distance),
    };
    const auto narrowed_point = narrow_vector(
        point,
        "Ray intersection point exceeds the representable Vector3 range");
    if (!narrowed_point) {
        return Result<RayHit3D>::failure(narrowed_point.error());
    }

    const auto narrowed_normal = narrow_vector(
        normal,
        "Ray intersection normal exceeds the representable Vector3 range");
    if (!narrowed_normal) {
        return Result<RayHit3D>::failure(narrowed_normal.error());
    }

    return Result<RayHit3D>::success(RayHit3D{
        .distance = distance,
        .point = narrowed_point.value(),
        .normal = narrowed_normal.value(),
    });
}

[[nodiscard]] bool valid_max_distance(double value) noexcept {
    return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] Result<std::optional<RayHit3D>> no_hit() {
    return Result<std::optional<RayHit3D>>::success(std::nullopt);
}

[[nodiscard]] Result<std::optional<RayHit3D>> hit_result(
    const Ray3D& ray,
    double distance,
    double max_distance,
    DoubleVector3 normal) {
    if (distance > max_distance) {
        return no_hit();
    }

    const auto hit = make_hit(ray, distance, normal);
    if (!hit) {
        return Result<std::optional<RayHit3D>>::failure(hit.error());
    }
    return Result<std::optional<RayHit3D>>::success(hit.value());
}

} // namespace

bool Aabb3D::is_valid() const noexcept {
    return minimum.is_finite() && maximum.is_finite() &&
        minimum.x <= maximum.x &&
        minimum.y <= maximum.y &&
        minimum.z <= maximum.z;
}

bool Aabb3D::contains(Vector3 point) const noexcept {
    return is_valid() && point.is_finite() &&
        point.x >= minimum.x && point.x <= maximum.x &&
        point.y >= minimum.y && point.y <= maximum.y &&
        point.z >= minimum.z && point.z <= maximum.z;
}

bool Aabb3D::intersects(const Aabb3D& other) const noexcept {
    return is_valid() && other.is_valid() &&
        minimum.x <= other.maximum.x && maximum.x >= other.minimum.x &&
        minimum.y <= other.maximum.y && maximum.y >= other.minimum.y &&
        minimum.z <= other.maximum.z && maximum.z >= other.minimum.z;
}

Result<Aabb3D> Aabb3D::from_center_half_extents(
    Vector3 center,
    Vector3 half_extents) {
    if (!center.is_finite() || !half_extents.is_finite() ||
        half_extents.x < 0.0F ||
        half_extents.y < 0.0F ||
        half_extents.z < 0.0F) {
        return Result<Aabb3D>::failure(invalid_geometry_input(
            "AABB center must be finite and half-extents must be finite and non-negative"));
    }

    const DoubleVector3 center_value = to_double(center);
    const DoubleVector3 extents_value = to_double(half_extents);
    const auto minimum = narrow_vector(
        DoubleVector3{
            center_value.x - extents_value.x,
            center_value.y - extents_value.y,
            center_value.z - extents_value.z,
        },
        "AABB minimum exceeds the representable Vector3 range");
    const auto maximum = narrow_vector(
        DoubleVector3{
            center_value.x + extents_value.x,
            center_value.y + extents_value.y,
            center_value.z + extents_value.z,
        },
        "AABB maximum exceeds the representable Vector3 range");
    if (!minimum) {
        return Result<Aabb3D>::failure(minimum.error());
    }
    if (!maximum) {
        return Result<Aabb3D>::failure(maximum.error());
    }

    return Result<Aabb3D>::success(Aabb3D{
        .minimum = minimum.value(),
        .maximum = maximum.value(),
    });
}

Result<Aabb3D> Aabb3D::merged(const Aabb3D& other) const {
    if (!is_valid() || !other.is_valid()) {
        return Result<Aabb3D>::failure(
            invalid_geometry_input("AABB merge requires two valid bounds"));
    }

    return Result<Aabb3D>::success(Aabb3D{
        .minimum = Vector3{
            std::min(minimum.x, other.minimum.x),
            std::min(minimum.y, other.minimum.y),
            std::min(minimum.z, other.minimum.z),
        },
        .maximum = Vector3{
            std::max(maximum.x, other.maximum.x),
            std::max(maximum.y, other.maximum.y),
            std::max(maximum.z, other.maximum.z),
        },
    });
}

bool Sphere3D::is_valid() const noexcept {
    return center.is_finite() && std::isfinite(radius) && radius >= 0.0F;
}

bool Sphere3D::contains(Vector3 point) const noexcept {
    if (!is_valid() || !point.is_finite()) {
        return false;
    }

    const DoubleVector3 difference{
        static_cast<double>(point.x) - static_cast<double>(center.x),
        static_cast<double>(point.y) - static_cast<double>(center.y),
        static_cast<double>(point.z) - static_cast<double>(center.z),
    };
    const double radius_value = static_cast<double>(radius);
    return dot(difference, difference) <= radius_value * radius_value;
}

Ray3D::Ray3D(Vector3 origin, Vector3 normalized_direction) noexcept
    : origin_(origin), direction_(normalized_direction) {
}

Result<Ray3D> Ray3D::from_direction(Vector3 origin, Vector3 direction) {
    if (!origin.is_finite() || !direction.is_finite()) {
        return Result<Ray3D>::failure(invalid_geometry_input(
            "Ray origin and direction must contain only finite values"));
    }

    const auto normalized = direction.normalized();
    if (!normalized) {
        return Result<Ray3D>::failure(normalized.error());
    }
    return Result<Ray3D>::success(Ray3D{origin, normalized.value()});
}

bool Ray3D::is_valid() const noexcept {
    if (!origin_.is_finite() || !direction_.is_finite()) {
        return false;
    }
    const double length_squared = direction_.length_squared();
    return std::isfinite(length_squared) &&
        std::fabs(length_squared - 1.0) <= kUnitLengthSquaredTolerance;
}

Vector3 Ray3D::origin() const noexcept {
    return origin_;
}

Vector3 Ray3D::direction() const noexcept {
    return direction_;
}

Plane3D::Plane3D(Vector3 normalized_normal, float offset) noexcept
    : normal_(normalized_normal), offset_(offset) {
}

Result<Plane3D> Plane3D::from_point_normal(Vector3 point, Vector3 normal) {
    if (!point.is_finite() || !normal.is_finite()) {
        return Result<Plane3D>::failure(invalid_geometry_input(
            "Plane point and normal must contain only finite values"));
    }

    const auto normalized = normal.normalized();
    if (!normalized) {
        return Result<Plane3D>::failure(normalized.error());
    }

    const double offset = -dot(to_double(normalized.value()), to_double(point));
    if (!is_representable_float(offset)) {
        return Result<Plane3D>::failure(invalid_geometry_input(
            "Plane offset exceeds the representable float range"));
    }

    return Result<Plane3D>::success(Plane3D{
        normalized.value(),
        static_cast<float>(offset),
    });
}

bool Plane3D::is_valid() const noexcept {
    if (!normal_.is_finite() || !std::isfinite(offset_)) {
        return false;
    }
    const double length_squared = normal_.length_squared();
    return std::isfinite(length_squared) &&
        std::fabs(length_squared - 1.0) <= kUnitLengthSquaredTolerance;
}

Vector3 Plane3D::normal() const noexcept {
    return normal_;
}

float Plane3D::offset() const noexcept {
    return offset_;
}

Result<double> Plane3D::signed_distance(Vector3 point) const {
    if (!is_valid() || !point.is_finite()) {
        return Result<double>::failure(invalid_geometry_input(
            "Signed plane distance requires a valid plane and finite point"));
    }

    const double distance =
        dot(to_double(normal_), to_double(point)) +
        static_cast<double>(offset_);
    if (!std::isfinite(distance)) {
        return Result<double>::failure(invalid_geometry_input(
            "Signed plane distance is not finite"));
    }
    return Result<double>::success(distance);
}

Result<std::optional<RayHit3D>> intersect_ray_aabb(
    const Ray3D& ray,
    const Aabb3D& bounds,
    double max_distance) {
    if (!ray.is_valid() || !bounds.is_valid() ||
        !valid_max_distance(max_distance)) {
        return Result<std::optional<RayHit3D>>::failure(invalid_geometry_input(
            "Ray-AABB intersection requires a valid ray, bounds, and finite non-negative distance"));
    }

    const DoubleVector3 origin = to_double(ray.origin());
    const DoubleVector3 direction = to_double(ray.direction());
    const std::array<double, 3U> origins{origin.x, origin.y, origin.z};
    const std::array<double, 3U> directions{
        direction.x, direction.y, direction.z};
    const std::array<double, 3U> minimums{
        static_cast<double>(bounds.minimum.x),
        static_cast<double>(bounds.minimum.y),
        static_cast<double>(bounds.minimum.z)};
    const std::array<double, 3U> maximums{
        static_cast<double>(bounds.maximum.x),
        static_cast<double>(bounds.maximum.y),
        static_cast<double>(bounds.maximum.z)};

    double near_distance = -std::numeric_limits<double>::infinity();
    double far_distance = std::numeric_limits<double>::infinity();
    DoubleVector3 near_normal{};

    for (std::size_t axis = 0U; axis < 3U; ++axis) {
        const double axis_origin = origins[axis];
        const double axis_direction = directions[axis];
        if (axis_direction == 0.0) {
            if (axis_origin < minimums[axis] ||
                axis_origin > maximums[axis]) {
                return no_hit();
            }
            continue;
        }

        const double first = (minimums[axis] - axis_origin) / axis_direction;
        const double second = (maximums[axis] - axis_origin) / axis_direction;
        const double axis_near = std::min(first, second);
        const double axis_far = std::max(first, second);

        if (axis_near > near_distance) {
            near_distance = axis_near;
            const double sign = axis_direction > 0.0 ? -1.0 : 1.0;
            near_normal = DoubleVector3{};
            if (axis == 0U) near_normal.x = sign;
            if (axis == 1U) near_normal.y = sign;
            if (axis == 2U) near_normal.z = sign;
        }

        far_distance = std::min(far_distance, axis_far);
        if (near_distance > far_distance) {
            return no_hit();
        }
    }

    if (far_distance < 0.0) {
        return no_hit();
    }

    const bool starts_inside = near_distance < 0.0;
    const double hit_distance = starts_inside ? 0.0 : near_distance;
    if (hit_distance > max_distance) {
        return no_hit();
    }

    if (starts_inside) {
        near_normal = DoubleVector3{};
    }
    return hit_result(ray, hit_distance, max_distance, near_normal);
}

Result<std::optional<RayHit3D>> intersect_ray_sphere(
    const Ray3D& ray,
    const Sphere3D& sphere,
    double max_distance) {
    if (!ray.is_valid() || !sphere.is_valid() ||
        !valid_max_distance(max_distance)) {
        return Result<std::optional<RayHit3D>>::failure(invalid_geometry_input(
            "Ray-sphere intersection requires a valid ray, sphere, and finite non-negative distance"));
    }

    const DoubleVector3 origin = to_double(ray.origin());
    const DoubleVector3 direction = to_double(ray.direction());
    const DoubleVector3 center = to_double(sphere.center);
    const DoubleVector3 offset{
        origin.x - center.x,
        origin.y - center.y,
        origin.z - center.z,
    };
    const double radius = static_cast<double>(sphere.radius);
    const double b = dot(offset, direction);
    const double c = dot(offset, offset) - (radius * radius);
    const double discriminant = (b * b) - c;
    if (!std::isfinite(discriminant)) {
        return Result<std::optional<RayHit3D>>::failure(invalid_geometry_input(
            "Ray-sphere intersection exceeded the finite numeric range"));
    }
    if (discriminant < 0.0) {
        return no_hit();
    }

    double distance = -b - std::sqrt(discriminant);
    if (distance < 0.0) {
        if (c > 0.0) {
            return no_hit();
        }
        distance = 0.0;
    }
    if (distance > max_distance) {
        return no_hit();
    }

    DoubleVector3 normal{};
    if (distance > 0.0 && radius > 0.0) {
        const DoubleVector3 point{
            origin.x + (direction.x * distance),
            origin.y + (direction.y * distance),
            origin.z + (direction.z * distance),
        };
        const DoubleVector3 difference{
            (point.x - center.x) / radius,
            (point.y - center.y) / radius,
            (point.z - center.z) / radius,
        };
        const double magnitude_squared = dot(difference, difference);
        if (!std::isfinite(magnitude_squared) ||
            magnitude_squared <= kMinimumMagnitudeSquared) {
            return Result<std::optional<RayHit3D>>::failure(
                invalid_geometry_input("Ray-sphere hit normal is invalid"));
        }
        const double inverse_magnitude = 1.0 / std::sqrt(magnitude_squared);
        normal = DoubleVector3{
            difference.x * inverse_magnitude,
            difference.y * inverse_magnitude,
            difference.z * inverse_magnitude,
        };
    }

    return hit_result(ray, distance, max_distance, normal);
}

Result<std::optional<RayHit3D>> intersect_ray_plane(
    const Ray3D& ray,
    const Plane3D& plane,
    double max_distance) {
    if (!ray.is_valid() || !plane.is_valid() ||
        !valid_max_distance(max_distance)) {
        return Result<std::optional<RayHit3D>>::failure(invalid_geometry_input(
            "Ray-plane intersection requires a valid ray, plane, and finite non-negative distance"));
    }

    const DoubleVector3 normal = to_double(plane.normal());
    const DoubleVector3 origin = to_double(ray.origin());
    const DoubleVector3 direction = to_double(ray.direction());
    const double denominator = dot(normal, direction);
    if (std::fabs(denominator) <= kParallelDirectionTolerance) {
        return no_hit();
    }

    const double numerator =
        dot(normal, origin) + static_cast<double>(plane.offset());
    const double distance = -numerator / denominator;
    if (!std::isfinite(distance)) {
        return Result<std::optional<RayHit3D>>::failure(invalid_geometry_input(
            "Ray-plane intersection produced a non-finite distance"));
    }
    if (distance < 0.0 || distance > max_distance) {
        return no_hit();
    }

    const double facing = denominator < 0.0 ? 1.0 : -1.0;
    const DoubleVector3 hit_normal{
        normal.x * facing,
        normal.y * facing,
        normal.z * facing,
    };
    return hit_result(ray, distance, max_distance, hit_normal);
}

} // namespace Storm::Math
