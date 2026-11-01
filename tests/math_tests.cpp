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
    EXPECT(Storm::Math::Vector3{1.0F, -2.0F, 3.0F}.is_finite());
    EXPECT(!Storm::Math::Vector3{
        std::numeric_limits<float>::infinity(), 0.0F, 0.0F}.is_finite());

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
        TestCase{"identity_quaternion_normalizes", &test_identity_quaternion_normalizes},
        TestCase{"quaternion_normalization", &test_quaternion_normalization},
        TestCase{"zero_quaternion_is_rejected", &test_zero_quaternion_is_rejected},
        TestCase{"axis_angle_rejects_invalid_inputs", &test_axis_angle_rejects_invalid_inputs},
        TestCase{"axis_angle_rotation_around_z", &test_axis_angle_rotation_around_z},
        TestCase{"transform_translation_and_scale", &test_transform_translation_and_scale},
        TestCase{"transform_uses_scale_then_rotation", &test_transform_uses_scale_then_rotation},
        TestCase{"matrix_multiplication_composes_parent_before_local", &test_matrix_multiplication_composes_parent_before_local},
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
