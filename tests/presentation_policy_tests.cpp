#include "storm/rendering/presentation_policy.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace {
#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)
using Storm::Rendering::PresentationExtent;
using Storm::Rendering::choose_presentation_image_count;
using Storm::Rendering::resolve_presentation_extent;

[[nodiscard]] std::optional<std::string> variable_extent_is_clamped() {
    constexpr std::uint32_t variable = UINT32_MAX;
    const auto result = resolve_presentation_extent(
        PresentationExtent{3000U, 100U},
        PresentationExtent{variable, variable},
        PresentationExtent{64U, 64U},
        PresentationExtent{2048U, 1536U});
    EXPECT(result);
    EXPECT(result.value() == (PresentationExtent{2048U, 100U}));
    return {};
}

[[nodiscard]] std::optional<std::string> fixed_extent_is_preserved() {
    const auto result = resolve_presentation_extent(
        PresentationExtent{900U, 700U},
        PresentationExtent{1280U, 720U},
        PresentationExtent{1U, 1U},
        PresentationExtent{4096U, 4096U});
    EXPECT(result);
    EXPECT(result.value() == (PresentationExtent{1280U, 720U}));
    return {};
}

[[nodiscard]] std::optional<std::string> malformed_extent_bounds_rejected() {
    constexpr std::uint32_t variable = UINT32_MAX;
    EXPECT(!resolve_presentation_extent(
        PresentationExtent{10U, 10U}, PresentationExtent{variable, 0U},
        PresentationExtent{1U, 1U}, PresentationExtent{100U, 100U}));
    EXPECT(!resolve_presentation_extent(
        PresentationExtent{10U, 10U}, PresentationExtent{variable, variable},
        PresentationExtent{10U, 10U}, PresentationExtent{9U, 20U}));
    EXPECT(!resolve_presentation_extent(
        PresentationExtent{10U, 10U}, PresentationExtent{20U, 20U},
        PresentationExtent{30U, 30U}, PresentationExtent{40U, 40U}));
    EXPECT(!resolve_presentation_extent(
        PresentationExtent{0U, 10U}, PresentationExtent{variable, variable},
        PresentationExtent{1U, 1U}, PresentationExtent{100U, 100U}));
    return {};
}

[[nodiscard]] std::optional<std::string> minimum_plus_one_is_preferred() {
    const auto result = choose_presentation_image_count(2U, 4U, 8U);
    EXPECT(result);
    EXPECT(result.value() == 3U);
    return {};
}

[[nodiscard]] std::optional<std::string> maximum_and_hard_limit_are_respected() {
    const auto surface_cap = choose_presentation_image_count(2U, 2U, 8U);
    EXPECT(surface_cap);
    EXPECT(surface_cap.value() == 2U);
    const auto engine_cap = choose_presentation_image_count(2U, 0U, 2U);
    EXPECT(engine_cap);
    EXPECT(engine_cap.value() == 2U);
    return {};
}

[[nodiscard]] std::optional<std::string> impossible_image_count_is_rejected() {
    EXPECT(!choose_presentation_image_count(0U, 0U, 8U));
    EXPECT(!choose_presentation_image_count(3U, 2U, 8U));
    const auto exhausted = choose_presentation_image_count(9U, 0U, 8U);
    EXPECT(!exhausted);
    EXPECT(exhausted.error().code == Storm::ErrorCode::ResourceExhausted);
    return {};
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"variable_extent_is_clamped", variable_extent_is_clamped},
        TestCase{"fixed_extent_is_preserved", fixed_extent_is_preserved},
        TestCase{"malformed_extent_bounds_rejected", malformed_extent_bounds_rejected},
        TestCase{"minimum_plus_one_is_preferred", minimum_plus_one_is_preferred},
        TestCase{"maximum_and_hard_limit_are_respected", maximum_and_hard_limit_are_respected},
        TestCase{"impossible_image_count_is_rejected", impossible_image_count_is_rejected},
    };
    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto result = test.run();
        if (result.has_value()) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << *result << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }
    std::cout << "Executed " << tests.size() << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
