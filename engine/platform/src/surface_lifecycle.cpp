#include "storm/platform/surface_lifecycle.hpp"

#include <cmath>
#include <limits>

namespace Storm::Platform {

Result<SurfaceDescriptor> SurfaceLifecycle::attach(
    NativeSurfaceId native_id,
    SurfaceExtent extent,
    double pixels_per_logical_unit) {
    const Result<void> validation = validate_surface(native_id, extent, pixels_per_logical_unit);
    if (!validation) {
        return Result<SurfaceDescriptor>::failure(validation.error());
    }

    if (current_.has_value()) {
        return Result<SurfaceDescriptor>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidState,
            "A surface is already attached; detach it before attaching a replacement"));
    }

    if (last_generation_ == std::numeric_limits<std::uint64_t>::max()) {
        return Result<SurfaceDescriptor>::failure(Storm::Error::make(
            Storm::ErrorCode::ResourceExhausted,
            "The surface generation counter is exhausted"));
    }

    ++last_generation_;
    current_ = SurfaceDescriptor{
        .native_id = native_id,
        .generation = last_generation_,
        .extent = extent,
        .pixels_per_logical_unit = pixels_per_logical_unit,
    };
    return Result<SurfaceDescriptor>::success(*current_);
}

Result<SurfaceDescriptor> SurfaceLifecycle::resize(
    NativeSurfaceId native_id,
    std::uint64_t generation,
    SurfaceExtent extent,
    double pixels_per_logical_unit) {
    const Result<void> validation = validate_surface(native_id, extent, pixels_per_logical_unit);
    if (!validation) {
        return Result<SurfaceDescriptor>::failure(validation.error());
    }

    if (!current_.has_value() || current_->native_id != native_id ||
        current_->generation != generation) {
        return Result<SurfaceDescriptor>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidState,
            "The resize callback refers to a detached or stale surface generation"));
    }

    current_->extent = extent;
    current_->pixels_per_logical_unit = pixels_per_logical_unit;
    return Result<SurfaceDescriptor>::success(*current_);
}

Result<void> SurfaceLifecycle::detach(
    NativeSurfaceId native_id,
    std::uint64_t generation) {
    if (native_id == 0U) {
        return Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "A native surface identity must be non-zero"));
    }

    if (!current_.has_value() || current_->native_id != native_id ||
        current_->generation != generation) {
        return Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidState,
            "The detach callback refers to a detached or stale surface generation"));
    }

    current_.reset();
    return Result<void>::success();
}

std::optional<SurfaceDescriptor> SurfaceLifecycle::current() const noexcept {
    return current_;
}

Result<void> SurfaceLifecycle::validate_surface(
    NativeSurfaceId native_id,
    SurfaceExtent extent,
    double pixels_per_logical_unit) {
    if (native_id == 0U) {
        return Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "A native surface identity must be non-zero"));
    }
    if (extent.width == 0U || extent.height == 0U) {
        return Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "Surface width and height must both be non-zero"));
    }
    if (!std::isfinite(pixels_per_logical_unit) || pixels_per_logical_unit <= 0.0) {
        return Result<void>::failure(Storm::Error::make(
            Storm::ErrorCode::InvalidArgument,
            "Surface pixel scale must be finite and greater than zero"));
    }
    return Result<void>::success();
}

} // namespace Storm::Platform
