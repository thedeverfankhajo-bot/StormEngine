#pragma once

#include "storm/math/matrix4.hpp"

#include <cmath>
#include <optional>

namespace storm::math {

// Clip-space depth differs between graphics APIs. Choose a range explicitly at the call site;
// a projection matrix should never guess which renderer/backend will consume it.
enum class ClipDepthRange {
    ZeroToOne,
    NegativeOneToOne
};

// Projection helpers use a right-handed camera looking down local -Z, column vectors, and
// column-major Matrix4 storage. They are math primitives, not an implicit graphics backend ABI.
class Projection final {
public:
    Projection() = delete;

    // Creates a finite right-handed perspective projection.
    //
    // vertical_fov_radians is the full vertical field of view in (0, pi), aspect_ratio is
    // viewport width / height, and 0 < near_plane < far_plane. The camera-space near/far
    // planes map to the explicitly selected normalized-device depth range. X and Y are not
    // flipped; backends that require a different framebuffer/Y convention must convert it.
    //
    // Returns no matrix for invalid enums, non-finite parameters, invalid plane ordering, or
    // arithmetic that would produce non-finite matrix entries.
    [[nodiscard]] static std::optional<Matrix4> perspective_rh(
        float vertical_fov_radians, float aspect_ratio, float near_plane, float far_plane,
        ClipDepthRange depth_range) noexcept {
        constexpr float pi = 3.14159265358979323846F;

        if (!std::isfinite(vertical_fov_radians) ||
            vertical_fov_radians <= 0.0F || vertical_fov_radians >= pi ||
            !std::isfinite(aspect_ratio) || aspect_ratio <= 0.0F ||
            !std::isfinite(near_plane) || near_plane <= 0.0F ||
            !std::isfinite(far_plane) || far_plane <= near_plane) {
            return std::nullopt;
        }

        if (depth_range != ClipDepthRange::ZeroToOne &&
            depth_range != ClipDepthRange::NegativeOneToOne) {
            return std::nullopt;
        }

        const float tangent = std::tan(vertical_fov_radians * 0.5F);
        if (!std::isfinite(tangent) || tangent <= 0.0F) {
            return std::nullopt;
        }

        const float focal_length = 1.0F / tangent;
        const float depth_denominator = near_plane - far_plane;
        if (!std::isfinite(focal_length) || focal_length <= 0.0F ||
            !std::isfinite(depth_denominator) || depth_denominator == 0.0F) {
            return std::nullopt;
        }

        Matrix4 result;
        result(0U, 0U) = focal_length / aspect_ratio;
        result(1U, 1U) = focal_length;
        result(2U, 2U) = depth_range == ClipDepthRange::ZeroToOne
                              ? far_plane / depth_denominator
                              : (far_plane + near_plane) / depth_denominator;
        result(2U, 3U) = depth_range == ClipDepthRange::ZeroToOne
                              ? (far_plane * near_plane) / depth_denominator
                              : (2.0F * far_plane * near_plane) / depth_denominator;
        result(3U, 2U) = -1.0F;

        return result.is_finite() ? std::optional<Matrix4>{result} : std::nullopt;
    }
};

} // namespace storm::math
