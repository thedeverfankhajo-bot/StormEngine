#include "storm/platform/surface_lifecycle.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {

#define EXPECT(condition) \
    do { \
        if (!(condition)) { \
            return std::string("Expectation failed: ") + #condition; \
        } \
    } while (false)

[[nodiscard]] std::optional<std::string> test_attach_validation_and_generation() {
    using Storm::Platform::SurfaceExtent;
    using Storm::Platform::SurfaceLifecycle;

    SurfaceLifecycle surface;
    auto zero_id = surface.attach(0U, SurfaceExtent{640U, 480U}, 1.0);
    EXPECT(!zero_id);
    EXPECT(zero_id.error().code == Storm::ErrorCode::InvalidArgument);

    auto zero_width = surface.attach(11U, SurfaceExtent{0U, 480U}, 1.0);
    EXPECT(!zero_width);
    EXPECT(zero_width.error().code == Storm::ErrorCode::InvalidArgument);

    auto invalid_scale = surface.attach(
        11U, SurfaceExtent{640U, 480U}, std::numeric_limits<double>::quiet_NaN());
    EXPECT(!invalid_scale);
    EXPECT(invalid_scale.error().code == Storm::ErrorCode::InvalidArgument);

    auto attached = surface.attach(11U, SurfaceExtent{640U, 480U}, 2.0);
    EXPECT(attached);
    EXPECT(attached.value().native_id == 11U);
    EXPECT(attached.value().generation == 1U);
    EXPECT(attached.value().extent.width == 640U);
    EXPECT(attached.value().extent.height == 480U);
    EXPECT(attached.value().pixels_per_logical_unit == 2.0);
    EXPECT(surface.current().has_value());

    auto duplicate = surface.attach(12U, SurfaceExtent{320U, 240U}, 1.0);
    EXPECT(!duplicate);
    EXPECT(duplicate.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(surface.current()->native_id == 11U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_resize_preserves_generation() {
    using Storm::Platform::SurfaceExtent;
    using Storm::Platform::SurfaceLifecycle;

    SurfaceLifecycle surface;
    auto attached = surface.attach(21U, SurfaceExtent{800U, 600U}, 1.0);
    EXPECT(attached);
    const std::uint64_t generation = attached.value().generation;

    auto resized = surface.resize(21U, generation, SurfaceExtent{1200U, 900U}, 2.0);
    EXPECT(resized);
    EXPECT(resized.value().generation == generation);
    EXPECT(resized.value().extent.width == 1200U);
    EXPECT(resized.value().extent.height == 900U);
    EXPECT(resized.value().pixels_per_logical_unit == 2.0);

    auto invalid_resize = surface.resize(21U, generation, SurfaceExtent{1200U, 0U}, 2.0);
    EXPECT(!invalid_resize);
    EXPECT(invalid_resize.error().code == Storm::ErrorCode::InvalidArgument);
    EXPECT(surface.current()->extent.height == 900U);

    auto stale_identity = surface.resize(22U, generation, SurfaceExtent{100U, 100U}, 1.0);
    EXPECT(!stale_identity);
    EXPECT(stale_identity.error().code == Storm::ErrorCode::InvalidState);

    auto stale_generation = surface.resize(21U, generation + 1U, SurfaceExtent{100U, 100U}, 1.0);
    EXPECT(!stale_generation);
    EXPECT(stale_generation.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(surface.current()->extent.width == 1200U);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_detach_and_recreation_reject_stale_callbacks() {
    using Storm::Platform::SurfaceExtent;
    using Storm::Platform::SurfaceLifecycle;

    SurfaceLifecycle surface;
    auto first = surface.attach(31U, SurfaceExtent{1080U, 1920U}, 3.0);
    EXPECT(first);
    const std::uint64_t old_generation = first.value().generation;

    auto stale_detach = surface.detach(32U, old_generation);
    EXPECT(!stale_detach);
    EXPECT(stale_detach.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(surface.current().has_value());

    EXPECT(surface.detach(31U, old_generation));
    EXPECT(!surface.current().has_value());

    auto second = surface.attach(41U, SurfaceExtent{1920U, 1080U}, 2.0);
    EXPECT(second);
    EXPECT(second.value().generation == old_generation + 1U);
    EXPECT(second.value().native_id == 41U);

    auto stale_resize = surface.resize(31U, old_generation, SurfaceExtent{1U, 1U}, 1.0);
    EXPECT(!stale_resize);
    EXPECT(stale_resize.error().code == Storm::ErrorCode::InvalidState);
    auto stale_detach_after_recreate = surface.detach(31U, old_generation);
    EXPECT(!stale_detach_after_recreate);
    EXPECT(surface.current()->native_id == 41U);
    EXPECT(surface.current()->generation == second.value().generation);
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> test_operations_without_surface_fail() {
    using Storm::Platform::SurfaceExtent;
    using Storm::Platform::SurfaceLifecycle;

    SurfaceLifecycle surface;
    auto resize = surface.resize(51U, 1U, SurfaceExtent{640U, 480U}, 1.0);
    EXPECT(!resize);
    EXPECT(resize.error().code == Storm::ErrorCode::InvalidState);

    auto detach = surface.detach(51U, 1U);
    EXPECT(!detach);
    EXPECT(detach.error().code == Storm::ErrorCode::InvalidState);
    EXPECT(!surface.current().has_value());
    return std::nullopt;
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr TestCase tests[]{
        {"attach_validation_and_generation", &test_attach_validation_and_generation},
        {"resize_preserves_generation", &test_resize_preserves_generation},
        {"detach_and_recreation_reject_stale_callbacks", &test_detach_and_recreation_reject_stale_callbacks},
        {"operations_without_surface_fail", &test_operations_without_surface_fail},
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

    std::cout << "Executed " << std::size(tests) << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
