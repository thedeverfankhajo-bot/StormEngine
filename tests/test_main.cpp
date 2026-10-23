#include "storm/core/version.hpp"
#include "storm/math/vector.hpp"
#include "storm/scene/dimension.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view expression, int line) {
    if (!condition) {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        ++failures;
    }
}

#define STORM_CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

void test_version() {
    STORM_CHECK(storm::version() == storm::kVersion);
    STORM_CHECK(storm::version() == (storm::Version{0, 1, 0}));
    STORM_CHECK(storm::version_string() == "0.1.0");
}

void test_vector_arithmetic() {
    using storm::math::Vec1;
    using storm::math::Vec2;
    using storm::math::Vec3;

    STORM_CHECK((Vec1{2.0F} + Vec1{3.0F}) == Vec1{5.0F});
    STORM_CHECK((Vec2{1.0F, 2.0F} + Vec2{3.0F, 4.0F}) == Vec2{4.0F, 6.0F});
    STORM_CHECK((Vec3{3.0F, 4.0F, 5.0F} - Vec3{1.0F, 2.0F, 3.0F}) ==
                Vec3{2.0F, 2.0F, 2.0F});
    STORM_CHECK((Vec2{3.0F, 4.0F}.dot(Vec2{3.0F, 4.0F})) == 25.0F);
    STORM_CHECK((Vec3{1.0F, 2.0F, 3.0F} * 2.0F) == Vec3{2.0F, 4.0F, 6.0F});
    STORM_CHECK((2.0F * Vec2{2.0F, 5.0F}) == Vec2{4.0F, 10.0F});

    auto accumulated = Vec2{1.0F, 2.0F};
    accumulated += Vec2{2.0F, 3.0F};
    STORM_CHECK((accumulated == Vec2{3.0F, 5.0F}));
    accumulated -= Vec2{1.0F, 1.0F};
    STORM_CHECK((accumulated == Vec2{2.0F, 4.0F}));
}

void test_vector_normalization_and_validation() {
    using storm::math::Vec2;

    const auto normalized = Vec2{3.0F, 4.0F}.normalized();
    STORM_CHECK(normalized.has_value());
    if (normalized.has_value()) {
        STORM_CHECK(std::abs(normalized->length() - 1.0F) < 1.0e-5F);
        STORM_CHECK(std::abs(normalized->operator[](0) - 0.6F) < 1.0e-5F);
        STORM_CHECK(std::abs(normalized->operator[](1) - 0.8F) < 1.0e-5F);
    }

    STORM_CHECK((!Vec2{0.0F, 0.0F}.normalized().has_value()));
    STORM_CHECK((!Vec2{1.0F, 0.0F}.normalized(-1.0F).has_value()));
    STORM_CHECK((!Vec2{1.0F, 0.0F}.normalized(
                     std::numeric_limits<float>::quiet_NaN()).has_value()));
    STORM_CHECK((!Vec2{std::numeric_limits<float>::infinity(), 1.0F}.is_finite()));
    STORM_CHECK((Vec2{1.0F, 1.0F}.is_finite()));
}

void test_dimensions() {
    using storm::scene::Dimension;
    using storm::scene::component_count;
    using storm::scene::dimension_name;
    using storm::scene::is_valid_dimension;

    STORM_CHECK(is_valid_dimension(Dimension::One));
    STORM_CHECK(is_valid_dimension(Dimension::Two));
    STORM_CHECK(is_valid_dimension(Dimension::Three));
    STORM_CHECK(!is_valid_dimension(static_cast<Dimension>(0)));
    STORM_CHECK(!is_valid_dimension(static_cast<Dimension>(4)));
    STORM_CHECK(component_count(Dimension::One) == 1U);
    STORM_CHECK(component_count(Dimension::Two) == 2U);
    STORM_CHECK(component_count(Dimension::Three) == 3U);
    STORM_CHECK(component_count(static_cast<Dimension>(255)) == 0U);
    STORM_CHECK(dimension_name(Dimension::One) == "1D");
    STORM_CHECK(dimension_name(Dimension::Two) == "2D");
    STORM_CHECK(dimension_name(Dimension::Three) == "3D");
    STORM_CHECK(dimension_name(static_cast<Dimension>(99)) == "Unknown");
}

} // namespace

int main() {
    test_version();
    test_vector_arithmetic();
    test_vector_normalization_and_validation();
    test_dimensions();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed.\n";
        return 1;
    }

    std::cout << "All StormEngine core tests passed.\n";
    return 0;
}
