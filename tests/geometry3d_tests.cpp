#include "storm/math/geometry3d.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] bool nearly_equal(
    double lhs,
    double rhs,
    double tolerance = 1.0e-5) {
    return std::fabs(lhs - rhs) <= tolerance;
}

[[nodiscard]] std::optional<std::string> test_aabb_validation_and_inclusive_overlap() {
    using Storm::Math::Aabb3D;
    using Storm::Math::Vector3;

    const Aabb3D first{
        .minimum = {-1.0F, -1.0F, -1.0F},
        .maximum = {1.0F, 1.0F, 1.0F},
    };
    const Aabb3D touching{
        .minimum = {1.0F, 0.0F, 0.0F},
        .maximum = {2.0F, 1.0F, 1.0F},
    };
    const Aabb3D separated{
        .minimum = {1.01F, 0.0F, 0.0F},
        .maximum = {2.0F, 1.0F, 1.0F},
    };

    EXPECT(first.is_valid());
    EXPECT(first.contains({0.0F, 0.0F, 0.0F}));
    EXPECT(first.contains({1.0F, 1.0F, 1.0F}));
    EXPECT(!first.contains({1.1F, 0.0F, 0.0F}));
    EXPECT(!first.contains(Vector3{
        std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F}));
    EXPECT(first.intersects(touching));
    EXPECT(!first.intersects(separated));

    const Aabb3D invalid{
        .minimum = {1.0F, 0.0F, 0.0F},
        .maximum = {-1.0F, 1.0F, 1.0F},
    };
    EXPECT(!invalid.is_valid());
    EXPECT(!first.intersects(invalid));
    EXPECT(!invalid.contains({0.0F, 0.0F, 0.0F}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_aabb_factory_merge_and_overflow() {
    using Storm::Math::Aabb3D;
    using Storm::Math::Vector3;

    const auto created = Aabb3D::from_center_half_extents(
        {2.0F, 0.0F, 0.0F}, {1.0F, 2.0F, 3.0F});
    EXPECT(created);
    EXPECT(nearly_equal(created.value().minimum.x, 1.0));
    EXPECT(nearly_equal(created.value().minimum.y, -2.0));
    EXPECT(nearly_equal(created.value().maximum.z, 3.0));

    const Aabb3D other{
        .minimum = {-4.0F, -1.0F, 0.0F},
        .maximum = {-2.0F, 1.0F, 2.0F},
    };
    const auto merged = created.value().merged(other);
    EXPECT(merged);
    EXPECT(nearly_equal(merged.value().minimum.x, -4.0));
    EXPECT(nearly_equal(merged.value().maximum.x, 3.0));

    const auto negative_extent = Aabb3D::from_center_half_extents(
        {}, {-1.0F, 0.0F, 0.0F});
    EXPECT(!negative_extent);
    EXPECT(negative_extent.error().code == Storm::ErrorCode::InvalidArgument);

    const auto overflow = Aabb3D::from_center_half_extents(
        {std::numeric_limits<float>::max(), 0.0F, 0.0F},
        {std::numeric_limits<float>::max(), 0.0F, 0.0F});
    EXPECT(!overflow);
    EXPECT(!created.value().merged(Aabb3D{}));
    EXPECT(!Aabb3D::from_center_half_extents(
        Vector3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F}, {}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_sphere_validation_and_contains() {
    using Storm::Math::Sphere3D;
    using Storm::Math::Vector3;

    const Sphere3D sphere{.center = {1.0F, 2.0F, 3.0F}, .radius = 2.0F};
    EXPECT(sphere.is_valid());
    EXPECT(sphere.contains({1.0F, 2.0F, 3.0F}));
    EXPECT(sphere.contains({3.0F, 2.0F, 3.0F}));
    EXPECT(!sphere.contains({3.01F, 2.0F, 3.0F}));
    EXPECT(!sphere.contains(Vector3{
        std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F}));
    EXPECT((!Sphere3D{.center = {}, .radius = -1.0F}.is_valid()));
    EXPECT((Sphere3D{.center = {}, .radius = 0.0F}.is_valid()));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_plane_factory_normalizes_and_validates() {
    using Storm::Math::Plane3D;
    using Storm::Math::Vector3;

    const auto plane = Plane3D::from_point_normal(
        {0.0F, 2.0F, 0.0F}, {0.0F, 5.0F, 0.0F});
    EXPECT(plane);
    EXPECT(plane.value().is_valid());
    EXPECT(nearly_equal(plane.value().normal().y, 1.0));
    EXPECT(nearly_equal(plane.value().offset(), -2.0));
    const auto distance = plane.value().signed_distance({0.0F, 3.0F, 0.0F});
    EXPECT(distance);
    EXPECT(nearly_equal(distance.value(), 1.0));
    EXPECT(!plane.value().signed_distance(Vector3{
        std::numeric_limits<float>::infinity(), 0.0F, 0.0F}));

    const auto zero_normal = Plane3D::from_point_normal({}, {});
    EXPECT(!zero_normal);
    EXPECT(zero_normal.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(!Plane3D::from_point_normal(
        Vector3{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_ray_factory_normalizes_and_rejects_invalid_input() {
    using Storm::Math::Ray3D;
    using Storm::Math::Vector3;

    const auto ray = Ray3D::from_direction({}, {4.0F, 0.0F, 0.0F});
    EXPECT(ray);
    EXPECT(ray.value().is_valid());
    EXPECT(nearly_equal(ray.value().direction().x, 1.0));
    EXPECT(nearly_equal(ray.value().direction().length(), 1.0));

    const auto zero = Ray3D::from_direction({}, {});
    EXPECT(!zero);
    EXPECT(zero.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(!Ray3D::from_direction(
        Vector3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F}));
    EXPECT(!Ray3D::from_direction(
        {}, Vector3{0.0F, std::numeric_limits<float>::quiet_NaN(), 0.0F}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_ray_aabb_hit_distance_normal_and_inside_origin() {
    using Storm::Math::Aabb3D;
    using Storm::Math::Ray3D;
    using Storm::Math::intersect_ray_aabb;

    const Aabb3D bounds{
        .minimum = {-1.0F, -1.0F, -1.0F},
        .maximum = {1.0F, 1.0F, 1.0F},
    };
    const auto ray = Ray3D::from_direction(
        {-3.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F});
    EXPECT(ray);

    const auto hit = intersect_ray_aabb(ray.value(), bounds, 2.0);
    EXPECT(hit);
    EXPECT(hit.value().has_value());
    EXPECT(nearly_equal(hit.value()->distance, 2.0));
    EXPECT(nearly_equal(hit.value()->point.x, -1.0));
    EXPECT(nearly_equal(hit.value()->normal.x, -1.0));

    const auto too_short = intersect_ray_aabb(ray.value(), bounds, 1.99);
    EXPECT(too_short);
    EXPECT(!too_short.value().has_value());

    const auto inside_ray = Ray3D::from_direction({}, {0.0F, 1.0F, 0.0F});
    EXPECT(inside_ray);
    const auto inside_hit = intersect_ray_aabb(
        inside_ray.value(), bounds, 0.0);
    EXPECT(inside_hit);
    EXPECT(inside_hit.value().has_value());
    EXPECT(nearly_equal(inside_hit.value()->distance, 0.0));
    EXPECT(nearly_equal(inside_hit.value()->normal.length(), 0.0));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_ray_aabb_parallel_miss_and_invalid_query() {
    using Storm::Math::Aabb3D;
    using Storm::Math::Ray3D;
    using Storm::Math::intersect_ray_aabb;

    const Aabb3D bounds{
        .minimum = {-1.0F, -1.0F, -1.0F},
        .maximum = {1.0F, 1.0F, 1.0F},
    };
    const auto ray = Ray3D::from_direction(
        {2.0F, -3.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    EXPECT(ray);
    const auto miss = intersect_ray_aabb(ray.value(), bounds, 20.0);
    EXPECT(miss);
    EXPECT(!miss.value().has_value());

    const auto invalid_distance = intersect_ray_aabb(
        ray.value(), bounds, std::numeric_limits<double>::infinity());
    EXPECT(!invalid_distance);
    EXPECT(invalid_distance.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(!intersect_ray_aabb(ray.value(), Aabb3D{}, 1.0));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_ray_sphere_hit_tangent_inside_and_miss() {
    using Storm::Math::Ray3D;
    using Storm::Math::Sphere3D;
    using Storm::Math::intersect_ray_sphere;

    const Sphere3D sphere{.center = {}, .radius = 1.0F};
    const auto ray = Ray3D::from_direction(
        {-3.0F, 0.0F, 0.0F}, {3.0F, 0.0F, 0.0F});
    EXPECT(ray);
    const auto hit = intersect_ray_sphere(ray.value(), sphere, 2.0);
    EXPECT(hit);
    EXPECT(hit.value().has_value());
    EXPECT(nearly_equal(hit.value()->distance, 2.0));
    EXPECT(nearly_equal(hit.value()->normal.x, -1.0));

    const auto tangent_ray = Ray3D::from_direction(
        {-3.0F, 1.0F, 0.0F}, {1.0F, 0.0F, 0.0F});
    EXPECT(tangent_ray);
    const auto tangent = intersect_ray_sphere(tangent_ray.value(), sphere, 3.0);
    EXPECT(tangent);
    EXPECT(tangent.value().has_value());
    EXPECT(nearly_equal(tangent.value()->distance, 3.0));
    EXPECT(nearly_equal(tangent.value()->normal.y, 1.0));

    const auto inside_ray = Ray3D::from_direction({}, {1.0F, 0.0F, 0.0F});
    EXPECT(inside_ray);
    const auto inside = intersect_ray_sphere(inside_ray.value(), sphere, 0.0);
    EXPECT(inside);
    EXPECT(inside.value().has_value());
    EXPECT(nearly_equal(inside.value()->distance, 0.0));
    EXPECT(nearly_equal(inside.value()->normal.length(), 0.0));

    const auto miss_ray = Ray3D::from_direction(
        {-3.0F, 3.0F, 0.0F}, {1.0F, 0.0F, 0.0F});
    EXPECT(miss_ray);
    const auto miss = intersect_ray_sphere(miss_ray.value(), sphere, 10.0);
    EXPECT(miss);
    EXPECT(!miss.value().has_value());
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_ray_plane_hit_oriented_normal_parallel_and_behind() {
    using Storm::Math::Plane3D;
    using Storm::Math::Ray3D;
    using Storm::Math::intersect_ray_plane;

    const auto plane = Plane3D::from_point_normal(
        {0.0F, 2.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    EXPECT(plane);

    const auto upward = Ray3D::from_direction({}, {0.0F, 5.0F, 0.0F});
    EXPECT(upward);
    const auto upward_hit = intersect_ray_plane(upward.value(), plane.value(), 2.0);
    EXPECT(upward_hit);
    EXPECT(upward_hit.value().has_value());
    EXPECT(nearly_equal(upward_hit.value()->distance, 2.0));
    EXPECT(nearly_equal(upward_hit.value()->point.y, 2.0));
    EXPECT(nearly_equal(upward_hit.value()->normal.y, -1.0));

    const auto downward = Ray3D::from_direction(
        {0.0F, 4.0F, 0.0F}, {0.0F, -1.0F, 0.0F});
    EXPECT(downward);
    const auto downward_hit = intersect_ray_plane(
        downward.value(), plane.value(), 2.0);
    EXPECT(downward_hit);
    EXPECT(downward_hit.value().has_value());
    EXPECT(nearly_equal(downward_hit.value()->normal.y, 1.0));

    const auto parallel = Ray3D::from_direction(
        {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F});
    EXPECT(parallel);
    const auto no_plane_hit = intersect_ray_plane(
        parallel.value(), plane.value(), 10.0);
    EXPECT(no_plane_hit);
    EXPECT(!no_plane_hit.value().has_value());

    const auto behind = Ray3D::from_direction(
        {0.0F, 4.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    EXPECT(behind);
    const auto behind_hit = intersect_ray_plane(
        behind.value(), plane.value(), 10.0);
    EXPECT(behind_hit);
    EXPECT(!behind_hit.value().has_value());
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"aabb_validation_and_inclusive_overlap", &test_aabb_validation_and_inclusive_overlap},
        TestCase{"aabb_factory_merge_and_overflow", &test_aabb_factory_merge_and_overflow},
        TestCase{"sphere_validation_and_contains", &test_sphere_validation_and_contains},
        TestCase{"plane_factory_normalizes_and_validates", &test_plane_factory_normalizes_and_validates},
        TestCase{"ray_factory_normalizes_and_rejects_invalid_input", &test_ray_factory_normalizes_and_rejects_invalid_input},
        TestCase{"ray_aabb_hit_distance_normal_and_inside_origin", &test_ray_aabb_hit_distance_normal_and_inside_origin},
        TestCase{"ray_aabb_parallel_miss_and_invalid_query", &test_ray_aabb_parallel_miss_and_invalid_query},
        TestCase{"ray_sphere_hit_tangent_inside_and_miss", &test_ray_sphere_hit_tangent_inside_and_miss},
        TestCase{"ray_plane_hit_oriented_normal_parallel_and_behind", &test_ray_plane_hit_oriented_normal_parallel_and_behind},
    };

    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto failure = test.run();
        if (failure.has_value()) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << *failure << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }

    std::cout << "Executed " << tests.size()
              << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
