#pragma once

#include "storm/core/game_loop.hpp"
#include "storm/platform/lifecycle.hpp"
#include "storm/platform/surface_lifecycle.hpp"

#include <chrono>
#include <cstdint>
#include <optional>

namespace Storm::Runtime {

enum class FrameDisposition {
    Executed,
    SkippedInactive,
    SkippedSurfaceUnavailable,
};

struct MobileFrameResult final {
    FrameDisposition disposition{FrameDisposition::SkippedInactive};
    FixedStepAdvance timing{};
};

// Connects the platform-neutral lifecycle/surface contracts to Core's loop.
// The owner supplies monotonic timestamps and application callbacks. This
// class does not own a native clock, event pump, thread, renderer, or surface.
class MobileFrameDriver final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using MonotonicNanoseconds = std::int64_t;

    [[nodiscard]] static Result<MobileFrameDriver> create(
        FixedStepConfig config,
        Platform::LifecycleStateMachine& lifecycle,
        Platform::SurfaceLifecycle& surface);

    MobileFrameDriver(const MobileFrameDriver&) = delete;
    MobileFrameDriver& operator=(const MobileFrameDriver&) = delete;
    MobileFrameDriver(MobileFrameDriver&&) noexcept = default;
    MobileFrameDriver& operator=(MobileFrameDriver&&) = delete;

    [[nodiscard]] Result<MobileFrameResult> tick(
        TimePoint now,
        IGameLoopCallbacks& callbacks);

    // Accepts a monotonic timestamp from a native frame source. The timestamp
    // epoch is opaque; values must be non-decreasing and from one clock source
    // until timing is reset by lifecycle/surface changes or reset_timing().
    [[nodiscard]] Result<MobileFrameResult> tick_at_monotonic_nanoseconds(
        MonotonicNanoseconds timestamp_nanoseconds,
        IGameLoopCallbacks& callbacks);

    void reset_timing() noexcept;

private:
    MobileFrameDriver(
        GameLoop game_loop,
        Platform::LifecycleStateMachine& lifecycle,
        Platform::SurfaceLifecycle& surface) noexcept;

    GameLoop game_loop_;
    Platform::LifecycleStateMachine& lifecycle_;
    Platform::SurfaceLifecycle& surface_;
    std::optional<MonotonicNanoseconds> previous_frame_timestamp_ns_;
    std::optional<std::uint64_t> previous_lifecycle_revision_;
    std::optional<std::uint64_t> previous_surface_generation_;
};

} // namespace Storm::Runtime
