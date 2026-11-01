#include "storm/runtime/mobile_frame_driver.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace Storm::Runtime {

MobileFrameDriver::MobileFrameDriver(
    GameLoop game_loop,
    Platform::LifecycleStateMachine& lifecycle,
    Platform::SurfaceLifecycle& surface) noexcept
    : game_loop_(std::move(game_loop)),
      lifecycle_(lifecycle),
      surface_(surface) {}

Result<MobileFrameDriver> MobileFrameDriver::create(
    FixedStepConfig config,
    Platform::LifecycleStateMachine& lifecycle,
    Platform::SurfaceLifecycle& surface) {
    Result<GameLoop> game_loop = GameLoop::create(std::move(config));
    if (!game_loop) {
        return Result<MobileFrameDriver>::failure(game_loop.error());
    }

    return Result<MobileFrameDriver>::success(MobileFrameDriver{
        std::move(game_loop).value(),
        lifecycle,
        surface,
    });
}

Result<MobileFrameResult> MobileFrameDriver::tick(
    TimePoint now,
    IGameLoopCallbacks& callbacks) {
    // Convert the clock epoch to nanoseconds before passing it to the shared
    // timestamp path. Use an exclusive upper bound because 2^63 is not a
    // representable int64_t value, even on platforms where long double has
    // only double precision.
    const std::chrono::duration<long double, std::nano> timestamp_nanoseconds{
        now.time_since_epoch()};
    const long double timestamp_count = timestamp_nanoseconds.count();
    const long double signed_limit = std::ldexp(1.0L, 63);
    if (!std::isfinite(timestamp_count) ||
        timestamp_count < -signed_limit ||
        timestamp_count >= signed_limit) {
        reset_timing();
        return Result<MobileFrameResult>::failure(Storm::Error::make(
            Storm::ErrorCode::ResourceExhausted,
            "MobileFrameDriver timestamp exceeds nanosecond range"));
    }

    return tick_at_monotonic_nanoseconds(
        static_cast<MonotonicNanoseconds>(timestamp_count),
        callbacks);
}

Result<MobileFrameResult> MobileFrameDriver::tick_at_monotonic_nanoseconds(
    MonotonicNanoseconds timestamp_nanoseconds,
    IGameLoopCallbacks& callbacks) {
    if (!lifecycle_.can_run_game_loop()) {
        reset_timing();
        return Result<MobileFrameResult>::success(MobileFrameResult{
            .disposition = FrameDisposition::SkippedInactive,
            .timing = {},
        });
    }

    const std::optional<Platform::SurfaceDescriptor> active_surface =
        surface_.current();
    if (!active_surface.has_value()) {
        reset_timing();
        return Result<MobileFrameResult>::success(MobileFrameResult{
            .disposition = FrameDisposition::SkippedSurfaceUnavailable,
            .timing = {},
        });
    }

    const std::uint64_t lifecycle_revision = lifecycle_.revision();
    const std::uint64_t surface_generation = active_surface->generation;
    const bool environment_changed =
        !previous_lifecycle_revision_.has_value() ||
        *previous_lifecycle_revision_ != lifecycle_revision ||
        !previous_surface_generation_.has_value() ||
        *previous_surface_generation_ != surface_generation;

    if (environment_changed) {
        game_loop_.reset_timing();
        previous_frame_timestamp_ns_.reset();
    }

    std::chrono::nanoseconds frame_delta{0};
    if (previous_frame_timestamp_ns_.has_value()) {
        const MonotonicNanoseconds previous =
            *previous_frame_timestamp_ns_;
        if (timestamp_nanoseconds < previous) {
            reset_timing();
            return Result<MobileFrameResult>::failure(Storm::Error::make(
                Storm::ErrorCode::InvalidArgument,
                "MobileFrameDriver received a non-monotonic timestamp"));
        }

        constexpr MonotonicNanoseconds maximum =
            std::numeric_limits<MonotonicNanoseconds>::max();
        if (previous < 0 && timestamp_nanoseconds > maximum + previous) {
            reset_timing();
            return Result<MobileFrameResult>::failure(Storm::Error::make(
                Storm::ErrorCode::ResourceExhausted,
                "MobileFrameDriver elapsed time exceeds nanosecond range"));
        }

        // The range check above makes this signed subtraction safe.
        frame_delta = std::chrono::nanoseconds{
            timestamp_nanoseconds - previous};
    }

    // Record the attempted frame before invoking callbacks. A failed frame is
    // never replayed with the same elapsed duration on the next call.
    previous_frame_timestamp_ns_ = timestamp_nanoseconds;
    previous_lifecycle_revision_ = lifecycle_revision;
    previous_surface_generation_ = surface_generation;

    const Result<FixedStepAdvance> frame = game_loop_.tick(frame_delta, callbacks);
    if (!frame) {
        return Result<MobileFrameResult>::failure(frame.error());
    }

    return Result<MobileFrameResult>::success(MobileFrameResult{
        .disposition = FrameDisposition::Executed,
        .timing = frame.value(),
    });
}

void MobileFrameDriver::reset_timing() noexcept {
    game_loop_.reset_timing();
    previous_frame_timestamp_ns_.reset();
    previous_lifecycle_revision_.reset();
    previous_surface_generation_.reset();
}

} // namespace Storm::Runtime
