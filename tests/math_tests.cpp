#include "storm/math/transform3d.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] bool nearly_equal(float lhs, float rhs, float tolerance = 1.0e-4F) {
    return std::fabs(lhs - rhs) <= tolerance;
}

[[nodiscard]] std::optional<std::string> test_vector_and_matrix_finiteness() {
    EXPECT((Storm::Math::Vector3{1.0F, -2.0F, 3.0F}.is_finite()));
    EXPECT((!Storm::Math::Vector3{
        std::numeric_limits<float>::infinity(), 0.0F, 0.0F}.is_finite()));

    const auto identity = Storm::Math::Matrix4::identity();
    EXPECT(identity.is_finite());
    for (std::size_t row = 0U; row < 4U; ++row) {
        for (std::size_t column = 0U; column < 4U; ++column) {
            EXPECT(nearly_equal(
                identity.at(row, column),
                row == column ? 1.0F : 0.0F));
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_vector3_operations_validate_numeric_range() {
    const Storm::Math::Vector3 lhs{1.0F, 2.0F, 3.0F};
    const Storm::Math::Vector3 rhs{4.0F, -5.0F, 6.0F};

    EXPECT(nearly_equal(static_cast<float>(lhs.dot(rhs)), 12.0F));
    const auto cross = lhs.cross(rhs);
    EXPECT(cross);
    EXPECT(nearly_equal(cross.value().x, 27.0F));
    EXPECT(nearly_equal(cross.value().y, 6.0F));
    EXPECT(nearly_equal(cross.value().z, -13.0F));

    const auto normalized = Storm::Math::Vector3{0.0F, 3.0F, 4.0F}.normalized();
    EXPECT(normalized);
    EXPECT(nearly_equal(normalized.value().x, 0.0F));
    EXPECT(nearly_equal(normalized.value().y, 0.6F));
    EXPECT(nearly_equal(normalized.value().z, 0.8F));
    EXPECT(nearly_equal(
        static_cast<float>(Storm::Math::Vector3{3.0F, 4.0F, 0.0F}.length()),
        5.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_vector3_rejects_zero_non_finite_and_overflow() {
    EXPECT(!Storm::Math::Vector3{}.normalized());
    EXPECT(!Storm::Math::Vector3{
        std::numeric_limits<float>::quiet_NaN(), 1.0F, 0.0F}.normalized());
    const auto overflow = Storm::Math::Vector3{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        0.0F}.cross(Storm::Math::Vector3{
            -std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            0.0F});
    EXPECT(!overflow);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_identity_quaternion_normalizes() {
    const auto normalized = Storm::Math::Quaternion::identity().normalized();
    EXPECT(normalized);
    EXPECT(nearly_equal(normalized.value().x, 0.0F));
    EXPECT(nearly_equal(normalized.value().y, 0.0F));
    EXPECT(nearly_equal(normalized.value().z, 0.0F));
    EXPECT(nearly_equal(normalized.value().w, 1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_quaternion_normalization() {
    const auto normalized = Storm::Math::Quaternion{0.0F, 0.0F, 0.0F, 4.0F}.normalized();
    EXPECT(normalized);
    EXPECT(nearly_equal(normalized.value().w, 1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_zero_quaternion_is_rejected() {
    const auto normalized = Storm::Math::Quaternion{0.0F, 0.0F, 0.0F, 0.0F}.normalized();
    EXPECT(!normalized);
    EXPECT(normalized.error().code == Storm::ErrorCode::InvalidArgument);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_axis_angle_rejects_invalid_inputs() {
    const auto zero_axis = Storm::Math::Quaternion::from_axis_angle(
        Storm::Math::Vector3{}, 1.0F);
    EXPECT(!zero_axis);
    EXPECT(zero_axis.error().code == Storm::ErrorCode::InvalidArgument);

    const auto infinite_angle = Storm::Math::Quaternion::from_axis_angle(
        Storm::Math::Vector3{1.0F, 0.0F, 0.0F},
        std::numeric_limits<float>::infinity());
    EXPECT(!infinite_angle);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_axis_angle_rotation_around_z() {
    const auto rotation = Storm::Math::Quaternion::from_axis_angle(
        Storm::Math::Vector3{0.0F, 0.0F, 1.0F},
        1.57079632679F);
    EXPECT(rotation);

    const Storm::Math::Transform3D transform{
        .translation = {},
        .rotation = rotation.value(),
        .scale = {1.0F, 1.0F, 1.0F},
    };
    const auto matrix = transform.to_matrix();
    EXPECT(matrix);
    EXPECT(nearly_equal(matrix.value().at(0U, 0U), 0.0F));
    EXPECT(nearly_equal(matrix.value().at(1U, 0U), 1.0F));
    EXPECT(nearly_equal(matrix.value().at(0U, 1U), -1.0F));
    EXPECT(nearly_equal(matrix.value().at(1U, 1U), 0.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_transform_translation_and_scale() {
    const Storm::Math::Transform3D transform{
        .translation = {5.0F, 6.0F, 7.0F},
        .rotation = Storm::Math::Quaternion::identity(),
        .scale = {2.0F, 3.0F, 4.0F},
    };
    const auto matrix = transform.to_matrix();
    EXPECT(matrix);
    EXPECT(nearly_equal(matrix.value().at(0U, 0U), 2.0F));
    EXPECT(nearly_equal(matrix.value().at(1U, 1U), 3.0F));
    EXPECT(nearly_equal(matrix.value().at(2U, 2U), 4.0F));
    EXPECT(nearly_equal(matrix.value().at(0U, 3U), 5.0F));
    EXPECT(nearly_equal(matrix.value().at(1U, 3U), 6.0F));
    EXPECT(nearly_equal(matrix.value().at(2U, 3U), 7.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_transform_uses_scale_then_rotation() {
    const auto rotation = Storm::Math::Quaternion::from_axis_angle(
        Storm::Math::Vector3{0.0F, 0.0F, 1.0F},
        1.57079632679F);
    EXPECT(rotation);

    const Storm::Math::Transform3D transform{
        .translation = {},
        .rotation = rotation.value(),
        .scale = {2.0F, 1.0F, 1.0F},
    };
    const auto matrix = transform.to_matrix();
    EXPECT(matrix);
    EXPECT(nearly_equal(matrix.value().at(1U, 0U), 2.0F));
    EXPECT(nearly_equal(matrix.value().at(0U, 1U), -1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_matrix_multiplication_composes_parent_before_local() {
    const Storm::Math::Transform3D parent{
        .translation = {10.0F, 0.0F, 0.0F},
    };
    const Storm::Math::Transform3D child{
        .translation = {0.0F, 2.0F, 0.0F},
    };
    const auto parent_matrix = parent.to_matrix();
    const auto child_matrix = child.to_matrix();
    EXPECT(parent_matrix);
    EXPECT(child_matrix);

    const Storm::Math::Matrix4 world = parent_matrix.value() * child_matrix.value();
    EXPECT(nearly_equal(world.at(0U, 3U), 10.0F));
    EXPECT(nearly_equal(world.at(1U, 3U), 2.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_perspective_rh_zero_to_one_projection() {
    const auto projection = Storm::Math::Matrix4::perspective_rh_zo(
        1.57079632679F, 1.0F, 1.0F, 10.0F);
    EXPECT(projection);
    EXPECT(projection.value().is_finite());
    EXPECT(nearly_equal(projection.value().at(0U, 0U), 1.0F));
    EXPECT(nearly_equal(projection.value().at(1U, 1U), 1.0F));
    EXPECT(nearly_equal(projection.value().at(2U, 2U), -10.0F / 9.0F));
    EXPECT(nearly_equal(projection.value().at(2U, 3U), -10.0F / 9.0F));
    EXPECT(nearly_equal(projection.value().at(3U, 2U), -1.0F));

    const auto near_point = projection.value().transform_point({0.0F, 0.0F, -1.0F});
    const auto far_point = projection.value().transform_point({0.0F, 0.0F, -10.0F});
    EXPECT(near_point);
    EXPECT(far_point);
    EXPECT(nearly_equal(near_point.value().z, 0.0F));
    EXPECT(nearly_equal(far_point.value().z, 1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_orthographic_rh_zero_to_one_projection() {
    const auto projection = Storm::Math::Matrix4::orthographic_rh_zo(
        -2.0F, 2.0F, -1.0F, 1.0F, 1.0F, 11.0F);
    EXPECT(projection);
    EXPECT(projection.value().is_finite());
    const auto near_corner = projection.value().transform_point({2.0F, 1.0F, -1.0F});
    const auto far_center = projection.value().transform_point({0.0F, 0.0F, -11.0F});
    EXPECT(near_corner);
    EXPECT(far_center);
    EXPECT(nearly_equal(near_corner.value().x, 1.0F));
    EXPECT(nearly_equal(near_corner.value().y, 1.0F));
    EXPECT(nearly_equal(near_corner.value().z, 0.0F));
    EXPECT(nearly_equal(far_center.value().z, 1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_look_at_rh_builds_orthonormal_view() {
    const auto view = Storm::Math::Matrix4::look_at_rh(
        {1.0F, 2.0F, 3.0F},
        {1.0F, 2.0F, 2.0F},
        {0.0F, 1.0F, 0.0F});
    EXPECT(view);
    const auto eye = view.value().transform_point({1.0F, 2.0F, 3.0F});
    const auto target = view.value().transform_point({1.0F, 2.0F, 2.0F});
    EXPECT(eye);
    EXPECT(target);
    EXPECT(nearly_equal(eye.value().x, 0.0F));
    EXPECT(nearly_equal(eye.value().y, 0.0F));
    EXPECT(nearly_equal(eye.value().z, 0.0F));
    EXPECT(nearly_equal(target.value().z, -1.0F));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_camera_matrices_reject_invalid_or_unrepresentable_inputs() {
    EXPECT(!Storm::Math::Matrix4::perspective_rh_zo(
        0.0F, 1.0F, 0.1F, 100.0F));
    EXPECT(!Storm::Math::Matrix4::perspective_rh_zo(
        1.0F, 0.0F, 0.1F, 100.0F));
    EXPECT(!Storm::Math::Matrix4::perspective_rh_zo(
        1.0F, 1.0F, 0.1F, 0.1F));
    EXPECT(!Storm::Math::Matrix4::perspective_rh_zo(
        1.0F, std::numeric_limits<float>::denorm_min(), 0.1F, 100.0F));
    EXPECT(!Storm::Math::Matrix4::orthographic_rh_zo(
        1.0F, 1.0F, -1.0F, 1.0F, 0.1F, 10.0F));
    EXPECT(!Storm::Math::Matrix4::orthographic_rh_zo(
        -1.0F, 1.0F, -1.0F, 1.0F, 10.0F, 1.0F));
    EXPECT(!Storm::Math::Matrix4::look_at_rh(
        {1.0F, 2.0F, 3.0F}, {1.0F, 2.0F, 3.0F}, {0.0F, 1.0F, 0.0F}));
    EXPECT(!Storm::Math::Matrix4::look_at_rh(
        {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 0.0F, -2.0F}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_transform_point_and_direction_are_distinct() {
    const Storm::Math::Transform3D transform{
        .translation = {4.0F, 5.0F, 6.0F},
    };
    const auto matrix = transform.to_matrix();
    EXPECT(matrix);
    const auto point = matrix.value().transform_point({1.0F, 2.0F, 3.0F});
    const auto direction = matrix.value().transform_direction({1.0F, 2.0F, 3.0F});
    EXPECT(point);
    EXPECT(direction);
    EXPECT(nearly_equal(point.value().x, 5.0F));
    EXPECT(nearly_equal(point.value().y, 7.0F));
    EXPECT(nearly_equal(point.value().z, 9.0F));
    EXPECT(nearly_equal(direction.value().x, 1.0F));
    EXPECT(nearly_equal(direction.value().y, 2.0F));
    EXPECT(nearly_equal(direction.value().z, 3.0F));

    const auto zero_w = Storm::Math::Matrix4::perspective_rh_zo(
        1.0F, 1.0F, 0.1F, 100.0F);
    EXPECT(zero_w);
    EXPECT(!zero_w.value().transform_point({1.0F, 2.0F, 0.0F}));
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_matrix_accessor_checks_bounds() {
    const auto identity = Storm::Math::Matrix4::identity();
    bool threw = false;
    try {
        static_cast<void>(identity.at(4U, 0U));
    } catch (const std::out_of_range&) {
        threw = true;
    }
    EXPECT(threw);
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"vector_and_matrix_finiteness", &test_vector_and_matrix_finiteness},
        TestCase{"vector3_operations_validate_numeric_range", &test_vector3_operations_validate_numeric_range},
        TestCase{"vector3_rejects_zero_non_finite_and_overflow", &test_vector3_rejects_zero_non_finite_and_overflow},
        TestCase{"identity_quaternion_normalizes", &test_identity_quaternion_normalizes},
        TestCase{"quaternion_normalization", &test_quaternion_normalization},
        TestCase{"zero_quaternion_is_rejected", &test_zero_quaternion_is_rejected},
        TestCase{"axis_angle_rejects_invalid_inputs", &test_axis_angle_rejects_invalid_inputs},
        TestCase{"axis_angle_rotation_around_z", &test_axis_angle_rotation_around_z},
        TestCase{"transform_translation_and_scale", &test_transform_translation_and_scale},
        TestCase{"transform_uses_scale_then_rotation", &test_transform_uses_scale_then_rotation},
        TestCase{"matrix_multiplication_composes_parent_before_local", &test_matrix_multiplication_composes_parent_before_local},
        TestCase{"perspective_rh_zero_to_one_projection", &test_perspective_rh_zero_to_one_projection},
        TestCase{"orthographic_rh_zero_to_one_projection", &test_orthographic_rh_zero_to_one_projection},
        TestCase{"look_at_rh_builds_orthonormal_view", &test_look_at_rh_builds_orthonormal_view},
        TestCase{"camera_matrices_reject_invalid_or_unrepresentable_inputs", &test_camera_matrices_reject_invalid_or_unrepresentable_inputs},
        TestCase{"transform_point_and_direction_are_distinct", &test_transform_point_and_direction_are_distinct},
        TestCase{"matrix_accessor_checks_bounds", &test_matrix_accessor_checks_bounds},
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
