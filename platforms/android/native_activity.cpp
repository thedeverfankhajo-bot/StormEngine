#include "storm/platform/android/lifecycle_adapter.hpp"
#include "storm/platform/surface_lifecycle.hpp"

#include <android/configuration.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <cstdint>
#include <limits>

namespace {

constexpr const char* kLogTag = "StormEngine";

struct NativeActivityState final {
    Storm::Platform::LifecycleStateMachine lifecycle;
    Storm::Platform::SurfaceLifecycle surface;
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

    const Storm::Result<void> detached =
        state.surface.detach(current->native_id, current->generation);
    if (!detached) {
        fail(state, detached.error());
        return false;
    }
    return true;
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
        case APP_CMD_LOST_FOCUS:
            static_cast<void>(apply_lifecycle_event(*state, ActivityEvent::FocusLost));
            break;
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
    app->userData = &state;
    app->onAppCmd = &handle_app_command;

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
    app->onAppCmd = nullptr;
    app->userData = nullptr;
}
