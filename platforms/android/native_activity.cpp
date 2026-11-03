#include "storm/platform/android/lifecycle_adapter.hpp"
#include "storm/platform/surface_lifecycle.hpp"
#include "storm/input/touch_input.hpp"

#include <android/configuration.h>
#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <time.h>
#include <utility>

namespace {

constexpr const char* kLogTag = "StormEngine";

struct NativeActivityState final {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
    std::optional<Storm::Input::TouchInput> touch_input;
    Storm::Platform::NativeSurfaceId next_surface_id{1U};
    bool surface_ids_exhausted{false};
    bool failed{false};
};

void fail(NativeActivityState& state, const Storm::Error& error) {
    state.failed = true;
    static_cast<void>(__android_log_write(
        ANDROID_LOG_ERROR, kLogTag, error.message.c_str()));
}

void fail(
    NativeActivityState& state,
    Storm::ErrorCode code,
    const char* message) {
    fail(state, Storm::Error::make(code, message));
}

[[nodiscard]] bool monotonic_timestamp_ns(std::uint64_t& timestamp_ns) {
    timespec value{};
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0 ||
        value.tv_sec < 0 || value.tv_nsec < 0 || value.tv_nsec >= 1000000000L) {
        return false;
    }
    constexpr std::uint64_t kNanosecondsPerSecond = 1000000000ULL;
    const auto seconds = static_cast<std::uint64_t>(value.tv_sec);
    const auto nanoseconds = static_cast<std::uint64_t>(value.tv_nsec);
    if (seconds > (std::numeric_limits<std::uint64_t>::max() - nanoseconds) /
                      kNanosecondsPerSecond) {
        return false;
    }
    timestamp_ns = seconds * kNanosecondsPerSecond + nanoseconds;
    return true;
}

[[nodiscard]] bool cancel_touch_input(
    NativeActivityState& state, std::uint64_t timestamp_ns) {
    if (!state.touch_input.has_value()) return true;
    auto cancelled = state.touch_input->cancel_all(timestamp_ns);
    if (!cancelled &&
        cancelled.error().code == Storm::ErrorCode::ResourceExhausted) {
        // Preserve the cancellation transition even when accumulated native
        // events filled the frame buffer. Older transitions are superseded
        // only when there is no room left to report all active-pointer cancels.
        state.touch_input->begin_frame();
        cancelled = state.touch_input->cancel_all(timestamp_ns);
    }
    if (!cancelled) {
        static_cast<void>(__android_log_write(
            ANDROID_LOG_WARN, kLogTag, cancelled.error().message.c_str()));
        return false;
    }
    return true;
}

[[nodiscard]] bool apply_lifecycle_event(
    NativeActivityState& state,
    Storm::Platform::Android::ActivityEvent event) {
    const Storm::Result<void> result =
        Storm::Platform::Android::apply_activity_event(state.lifecycle, event);
    if (!result) {
        fail(state, result.error());
        return false;
    }
    return true;
}

[[nodiscard]] bool read_surface_metrics(
    android_app* app,
    NativeActivityState& state,
    Storm::Platform::SurfaceExtent& extent,
    double& pixels_per_logical_unit) {
    if (app == nullptr || app->window == nullptr) {
        fail(
            state,
            Storm::ErrorCode::InvalidState,
            "Android reported a surface event without a valid ANativeWindow");
        return false;
    }

    const std::int32_t width = ANativeWindow_getWidth(app->window);
    const std::int32_t height = ANativeWindow_getHeight(app->window);
    if (width <= 0 || height <= 0) {
        fail(
            state,
            Storm::ErrorCode::IoFailure,
            "ANativeWindow returned a non-positive surface extent");
        return false;
    }

    extent = Storm::Platform::SurfaceExtent{
        .width = static_cast<std::uint32_t>(width),
        .height = static_cast<std::uint32_t>(height),
    };

    // Android's default/any/none density values do not describe a concrete
    // pixel density. Keep the contract's neutral scale in those cases.
    pixels_per_logical_unit = 1.0;
    if (app->config != nullptr) {
        constexpr std::int32_t kDensityAny = 0xfffe;
        constexpr std::int32_t kDensityNone = 0xffff;
        constexpr double kDefaultDensityDpi = 160.0;
        const std::int32_t density_dpi = AConfiguration_getDensity(app->config);
        if (density_dpi > 0 &&
            density_dpi != kDensityAny &&
            density_dpi != kDensityNone) {
            pixels_per_logical_unit =
                static_cast<double>(density_dpi) / kDefaultDensityDpi;
        }
    }

    return true;
}

[[nodiscard]] bool attach_surface(
    android_app* app,
    NativeActivityState& state) {
    if (state.surface_ids_exhausted) {
        fail(
            state,
            Storm::ErrorCode::ResourceExhausted,
            "The Android native surface identity counter is exhausted");
        return false;
    }

    Storm::Platform::SurfaceExtent extent{};
    double pixels_per_logical_unit = 1.0;
    if (!read_surface_metrics(app, state, extent, pixels_per_logical_unit)) {
        return false;
    }

    const Storm::Platform::NativeSurfaceId native_id = state.next_surface_id;
    const Storm::Result<Storm::Platform::SurfaceDescriptor> attached =
        state.surface.attach(native_id, extent, pixels_per_logical_unit);
    if (!attached) {
        fail(state, attached.error());
        return false;
    }

    if (!state.touch_input.has_value()) {
        fail(state, Storm::ErrorCode::InvalidState,
             "Android touch input was not initialized before surface attachment");
        static_cast<void>(state.surface.detach(native_id, attached.value().generation));
        return false;
    }
    const auto input_attached = state.touch_input->attach_surface(attached.value().generation);
    if (!input_attached) {
        fail(state, input_attached.error());
        static_cast<void>(state.surface.detach(native_id, attached.value().generation));
        return false;
    }

    if (state.next_surface_id == std::numeric_limits<std::uint64_t>::max()) {
        state.surface_ids_exhausted = true;
    } else {
        ++state.next_surface_id;
    }

    return true;
}

[[nodiscard]] bool resize_surface(
    android_app* app,
    NativeActivityState& state) {
    const auto current = state.surface.current();
    if (!current.has_value() || app == nullptr || app->window == nullptr) {
        return true;
    }

    Storm::Platform::SurfaceExtent extent{};
    double pixels_per_logical_unit = 1.0;
    if (!read_surface_metrics(app, state, extent, pixels_per_logical_unit)) {
        return false;
    }

    // Coordinates are logical units. Cancel active pointers before changing
    // density so one gesture cannot mix two coordinate spaces.
    if (pixels_per_logical_unit != current->pixels_per_logical_unit ||
        extent.width != current->extent.width ||
        extent.height != current->extent.height) {
        std::uint64_t timestamp_ns = 0U;
        if (!monotonic_timestamp_ns(timestamp_ns) ||
            !cancel_touch_input(state, timestamp_ns)) {
            fail(state, Storm::ErrorCode::InvalidState,
                 "Could not safely cancel Android pointers before a density change");
            return false;
        }
    }

    const auto resized = state.surface.resize(
        current->native_id,
        current->generation,
        extent,
        pixels_per_logical_unit);
    if (!resized) {
        fail(state, resized.error());
        return false;
    }
    return true;
}

[[nodiscard]] bool detach_surface(NativeActivityState& state) {
    const auto current = state.surface.current();
    if (!current.has_value()) {
        return true;
    }

    std::uint64_t timestamp_ns = 0U;
    if (!monotonic_timestamp_ns(timestamp_ns)) {
        fail(state, Storm::ErrorCode::Internal,
             "Could not read a monotonic timestamp while detaching Android input");
        return false;
    }
    if (state.touch_input.has_value() && state.touch_input->surface_generation() ==
            std::optional<std::uint64_t>{current->generation}) {
        auto input_detached =
            state.touch_input->detach_surface(current->generation, timestamp_ns);
        if (!input_detached &&
            input_detached.error().code == Storm::ErrorCode::ResourceExhausted) {
            // A detach must report Cancel for every active pointer. If the
            // accumulated event buffer is full, discard old moves and retry.
            state.touch_input->begin_frame();
            input_detached =
                state.touch_input->detach_surface(current->generation, timestamp_ns);
        }
        if (!input_detached) {
            fail(state, input_detached.error());
            return false;
        }
    }

    const Storm::Result<void> detached =
        state.surface.detach(current->native_id, current->generation);
    if (!detached) {
        fail(state, detached.error());
        return false;
    }
    return true;
}


std::int32_t handle_input_event(android_app* app, AInputEvent* event) {
    if (app == nullptr || app->userData == nullptr || event == nullptr) return 0;
    auto* state = static_cast<NativeActivityState*>(app->userData);
    if (state->failed || !state->touch_input.has_value() ||
        AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    const auto surface = state->surface.current();
    if (!surface.has_value() ||
        state->touch_input->surface_generation() !=
            std::optional<std::uint64_t>{surface->generation} ||
        !std::isfinite(surface->pixels_per_logical_unit) ||
        surface->pixels_per_logical_unit <= 0.0) {
        return 0;
    }

    const std::int64_t signed_timestamp = AMotionEvent_getEventTime(event);
    if (signed_timestamp < 0) return 0;
    const auto timestamp_ns = static_cast<std::uint64_t>(signed_timestamp);
    const std::int32_t action = AMotionEvent_getAction(event);
    const std::int32_t masked_action = action & AMOTION_EVENT_ACTION_MASK;
    const std::size_t pointer_count = AMotionEvent_getPointerCount(event);
    if (pointer_count == 0U ||
        pointer_count > Storm::Input::TouchInput::maximum_supported_pointers) {
        static_cast<void>(cancel_touch_input(*state, timestamp_ns));
        return 1;
    }

    using Storm::Input::PointerEvent;
    using Storm::Input::PointerPhase;
    constexpr std::size_t kMaximumPointers =
        Storm::Input::TouchInput::maximum_supported_pointers;
    std::array<PointerEvent, kMaximumPointers> events{};
    std::size_t event_count = 0U;
    PointerPhase phase = PointerPhase::Move;
    std::size_t first_index = 0U;
    std::size_t last_index = pointer_count;

    switch (masked_action) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            phase = PointerPhase::Down;
            first_index = static_cast<std::size_t>(
                (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            if (first_index >= pointer_count) return 0;
            last_index = first_index + 1U;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            phase = PointerPhase::Move;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            phase = PointerPhase::Up;
            first_index = static_cast<std::size_t>(
                (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            if (first_index >= pointer_count) return 0;
            last_index = first_index + 1U;
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            return cancel_touch_input(*state, timestamp_ns) ? 1 : 0;
        default:
            return 0;
    }

    for (std::size_t index = first_index; index < last_index; ++index) {
        const std::int32_t tool_type = AMotionEvent_getToolType(event, index);
        if (tool_type != AMOTION_EVENT_TOOL_TYPE_UNKNOWN &&
            tool_type != AMOTION_EVENT_TOOL_TYPE_FINGER &&
            tool_type != AMOTION_EVENT_TOOL_TYPE_STYLUS &&
            tool_type != AMOTION_EVENT_TOOL_TYPE_ERASER) {
            continue; // Ignore mouse, palm, and non-touch motion sources.
        }

        const std::int32_t native_pointer_id =
            AMotionEvent_getPointerId(event, index);
        if (native_pointer_id < 0) {
            static_cast<void>(cancel_touch_input(*state, timestamp_ns));
            return 0;
        }

        const double x_pixels = static_cast<double>(AMotionEvent_getX(event, index));
        const double y_pixels = static_cast<double>(AMotionEvent_getY(event, index));
        events[event_count++] = PointerEvent{
            .pointer_id = static_cast<Storm::Input::PointerId>(native_pointer_id),
            .phase = phase,
            .x = x_pixels / surface->pixels_per_logical_unit,
            .y = y_pixels / surface->pixels_per_logical_unit,
            .timestamp_ns = timestamp_ns,
            .surface_generation = surface->generation,
        };
    }

    if (event_count == 0U) return 0;
    // Native callbacks are asynchronous relative to the display frame. Keep
    // appending transitions until TouchInputGameLoopAdapter consumes them.
    const auto processed = state->touch_input->process_batch(
        std::span<const PointerEvent>{events.data(), event_count});
    if (!processed) {
        static_cast<void>(__android_log_write(
            ANDROID_LOG_WARN, kLogTag, processed.error().message.c_str()));
        return 0;
    }
    return 1;
}

void handle_app_command(android_app* app, std::int32_t command) {
    if (app == nullptr || app->userData == nullptr) {
        static_cast<void>(__android_log_write(
            ANDROID_LOG_ERROR, kLogTag, "NativeActivity callback has no adapter state"));
        return;
    }

    auto* state = static_cast<NativeActivityState*>(app->userData);
    if (state->failed) {
        return;
    }

    using Storm::Platform::Android::ActivityEvent;
    switch (command) {
        case APP_CMD_START:
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::Started));
            break;
        case APP_CMD_RESUME:
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::Resumed));
            break;
        case APP_CMD_PAUSE:
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::Paused));
            break;
        case APP_CMD_STOP:
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::Stopped));
            break;
        case APP_CMD_GAINED_FOCUS:
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::FocusGained));
            break;
        case APP_CMD_LOST_FOCUS: {
            std::uint64_t timestamp_ns = 0U;
            if (monotonic_timestamp_ns(timestamp_ns)) {
                static_cast<void>(cancel_touch_input(*state, timestamp_ns));
            }
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::FocusLost));
            break;
        }
        case APP_CMD_INIT_WINDOW:
            static_cast<void>(attach_surface(app, *state));
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
            static_cast<void>(resize_surface(app, *state));
            break;
        case APP_CMD_TERM_WINDOW:
            static_cast<void>(detach_surface(*state));
            break;
        case APP_CMD_DESTROY:
            if (detach_surface(*state)) {
                static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::Destroyed));
            }
            break;
        default:
            // Input queues, low-memory policy, and other commands are not yet
            // part of this lifecycle-only adapter.
            break;
    }
}

} // namespace

extern "C" void android_main(android_app* app) {
    if (app == nullptr) {
        return;
    }

    NativeActivityState state{};
    auto input = Storm::Input::TouchInput::create();
    if (!input) {
        static_cast<void>(__android_log_write(
            ANDROID_LOG_ERROR, kLogTag, input.error().message.c_str()));
        return;
    }
    state.touch_input.emplace(std::move(input).value());
    app->userData = &state;
    app->onAppCmd = &handle_app_command;
    app->onInputEvent = &handle_input_event;

    // This is intentionally an event pump, not a game loop. The engine does
    // not yet have frame scheduling or gameplay updates.
    while (app->destroyRequested == 0 && !state.failed) {
        android_poll_source* source = nullptr;
        const int identifier = ALooper_pollOnce(
            -1,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&source));

        if (identifier == ALOOPER_POLL_ERROR) {
            fail(
                state,
                Storm::ErrorCode::Internal,
                "ALooper_pollOnce failed while waiting for NativeActivity events");
            break;
        }

        if (identifier >= 0 && source != nullptr) {
            source->process(app, source);
        }
    }

    static_cast<void>(detach_surface(state));
    app->onInputEvent = nullptr;
    app->onAppCmd = nullptr;
    app->userData = nullptr;
}
