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
        previous_frame_time_.reset();
    }

    std::chrono::nanoseconds frame_delta{0};
    if (previous_frame_time_.has_value()) {
        if (now < *previous_frame_time_) {
            reset_timing();
            return Result<MobileFrameResult>::failure(Storm::Error::make(
                Storm::ErrorCode::InvalidArgument,
                "MobileFrameDriver received a non-monotonic timestamp"));
        }

        // Convert each epoch duration before subtraction. Subtracting two
        // extreme steady_clock durations directly could overflow their native
        // representation before a range check gets a chance to run.
        const std::chrono::duration<long double, std::nano> now_nanoseconds{
            now.time_since_epoch()};
        const std::chrono::duration<long double, std::nano> previous_nanoseconds{
            previous_frame_time_->time_since_epoch()};
        const long double elapsed_count =
            now_nanoseconds.count() - previous_nanoseconds.count();
        const long double maximum_count = static_cast<long double>(
            std::numeric_limits<std::chrono::nanoseconds::rep>::max());
        if (!std::isfinite(elapsed_count) ||
            elapsed_count < 0.0L ||
            elapsed_count > maximum_count) {
            reset_timing();
            return Result<MobileFrameResult>::failure(Storm::Error::make(
                Storm::ErrorCode::ResourceExhausted,
                "MobileFrameDriver elapsed time exceeds nanosecond range"));
        }

        frame_delta = std::chrono::nanoseconds{
            static_cast<std::chrono::nanoseconds::rep>(elapsed_count)};
    }

    // Record the attempted frame before invoking callbacks. A failed frame is
    // never replayed with the same elapsed duration on the next call.
    previous_frame_time_ = now;
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
    previous_frame_time_.reset();
    previous_lifecycle_revision_.reset();
    previous_surface_generation_.reset();
}

} // namespace Storm::Runtime
