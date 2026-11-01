#pragma once

#include "storm/foundation/result.hpp"

#include <cstdint>
#include <optional>

namespace Storm::Platform {

using NativeSurfaceId = std::uint64_t;

struct SurfaceExtent final {
    std::uint32_t width{0U};
    std::uint32_t height{0U};
};

// Converts a logical point size to a checked pixel extent. Dimensions are
// rounded to the nearest pixel; invalid and unrepresentable sizes are rejected.
[[nodiscard]] Result<SurfaceExtent> surface_extent_from_logical_size(
    double logical_width,
    double logical_height,
    double pixels_per_logical_unit);

// A non-owning description of one native presentation surface lifetime.
// native_id is an opaque identity token, never a raw native pointer or handle.
struct SurfaceDescriptor final {
    NativeSurfaceId native_id{0U};
    std::uint64_t generation{0U};
    SurfaceExtent extent{};
    double pixels_per_logical_unit{1.0};
};

// Single-thread-affine surface lifecycle contract. Native adapters must
// serialize callbacks before calling this object. The adapter retains native
// ownership; this class only validates identity and lifetime generations.
class SurfaceLifecycle final {
public:
    SurfaceLifecycle() = default;

    [[nodiscard]] Result<SurfaceDescriptor> attach(
        NativeSurfaceId native_id,
        SurfaceExtent extent,
        double pixels_per_logical_unit);

    [[nodiscard]] Result<SurfaceDescriptor> resize(
        NativeSurfaceId native_id,
        std::uint64_t generation,
        SurfaceExtent extent,
        double pixels_per_logical_unit);

    [[nodiscard]] Result<void> detach(
        NativeSurfaceId native_id,
        std::uint64_t generation);

    [[nodiscard]] std::optional<SurfaceDescriptor> current() const noexcept;

private:
    [[nodiscard]] static Result<void> validate_surface(
        NativeSurfaceId native_id,
        SurfaceExtent extent,
        double pixels_per_logical_unit);

    std::optional<SurfaceDescriptor> current_;
    std::uint64_t last_generation_{0U};
};

} // namespace Storm::Platform
