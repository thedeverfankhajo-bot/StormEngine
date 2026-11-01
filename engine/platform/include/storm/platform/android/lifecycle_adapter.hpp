#pragma once

#include "storm/foundation/result.hpp"
#include "storm/platform/lifecycle.hpp"

namespace Storm::Platform::Android {

// Normalized events emitted by Android NativeActivity / Native App Glue.
// This header deliberately does not include Android NDK headers so the
// transition policy can be tested on host toolchains.
enum class ActivityEvent {
    Started,
    Resumed,
    Paused,
    Stopped,
    FocusGained,
    FocusLost,
    Destroyed,
};

// Applies one serialized Android activity event to the platform-neutral
// lifecycle state machine. Call this on the adapter's owning thread.
[[nodiscard]] Result<void> apply_activity_event(
    LifecycleStateMachine& lifecycle,
    ActivityEvent event);

} // namespace Storm::Platform::Android
