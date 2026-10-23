#include "storm/core/fixed_step_clock.hpp"
#include "storm/math/vector.hpp"
#include "storm/render/camera2d.hpp"
#include "storm/render/software_canvas2d.hpp"
#include "storm/render/scene_renderer2d.hpp"
#include "storm/scene/scene.hpp"
#include "storm/scene/transform.hpp"
#include "storm/platform/event_queue.hpp"
#include "storm/platform/input_state.hpp"
#include "storm/platform/key_code.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {

using storm::core::FixedStepClock;
using storm::math::Vec2;
using storm::platform::InputState;
using storm::platform::PlatformEvent;
using storm::platform::PlatformEventQueue;
using storm::render::Camera2D;
using storm::render::Rgba8;
using storm::render::SoftwareCanvas2D;
using storm::render::SceneRenderer2D;
using storm::scene::Dimension;
using storm::scene::Scene;
using storm::scene::Renderable2D;
using storm::scene::Transform;
using storm::scene::Transform2D;

constexpr int window_initial_width = 960;
constexpr int window_initial_height = 540;
constexpr float camera_initial_view_height = 9.0F;
constexpr float camera_minimum_zoom = 0.35F;
constexpr float camera_maximum_zoom = 3.0F;
constexpr float player_initial_size = 0.8F;
constexpr float player_speed_units_per_second = 4.3333335F;
constexpr double nanoseconds_per_second = 1'000'000'000.0;
constexpr Rgba8 background_color{13U, 19U, 34U, 255U};
constexpr Rgba8 player_color{72U, 194U, 164U, 255U};

[[nodiscard]] bool enqueue(PlatformEventQueue<>& queue,
                           const PlatformEvent& event) noexcept {
    return queue.push(event);
}

void print_sdl_error(const char* operation);

[[nodiscard]] bool ensure_software_surface(SDL_Renderer* renderer,
                                            SoftwareCanvas2D& canvas,
                                            SDL_Texture*& texture,
                                            int width,
                                            int height) {
    if (width < 0 || height < 0) {
        return false;
    }

    const auto canvas_width = static_cast<std::uint32_t>(width);
    const auto canvas_height = static_cast<std::uint32_t>(height);
    if (canvas.width() != canvas_width || canvas.height() != canvas_height) {
        const auto resized = canvas.resize(canvas_width, canvas_height);
        if (!resized.ok()) {
            std::cerr << "software canvas resize failed: " << resized.message() << '\n';
            return false;
        }
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    // A minimized window may temporarily have a zero-sized surface.
    if (canvas_width == 0U || canvas_height == 0U) {
        return true;
    }
    if (texture == nullptr) {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_STREAMING, width, height);
        if (texture == nullptr) {
            print_sdl_error("SDL_CreateTexture");
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool render_frame(SDL_Renderer* renderer, SDL_Texture* texture,
                                SoftwareCanvas2D& canvas, const Camera2D& camera,
                                const Scene& scene, int width, int height) {
    if (width <= 0 || height <= 0) {
        return true;
    }
    if (texture == nullptr || canvas.width() != static_cast<std::uint32_t>(width) ||
        canvas.height() != static_cast<std::uint32_t>(height) ||
        camera.viewport_width != static_cast<std::uint32_t>(width) ||
        camera.viewport_height != static_cast<std::uint32_t>(height)) {
        return false;
    }

    const auto draw_status =
        SceneRenderer2D::render(scene, camera, canvas, background_color, player_color);
    if (!draw_status.ok()) {
        std::cerr << "2D scene render failed: " << draw_status.message() << '\n';
        return false;
    }

    void* locked_pixels = nullptr;
    int locked_pitch = 0;
    if (!SDL_LockTexture(texture, nullptr, &locked_pixels, &locked_pitch)) {
        return false;
    }

    const std::size_t source_pitch =
        static_cast<std::size_t>(canvas.width()) * sizeof(Rgba8);
    if (locked_pixels == nullptr || locked_pitch < 0 ||
        static_cast<std::size_t>(locked_pitch) < source_pitch) {
        SDL_UnlockTexture(texture);
        return false;
    }

    auto* destination_bytes = static_cast<std::byte*>(locked_pixels);
    const auto source_pixels = canvas.pixels();
    for (std::size_t row = 0U; row < static_cast<std::size_t>(canvas.height()); ++row) {
        std::memcpy(destination_bytes + row * static_cast<std::size_t>(locked_pitch),
                    source_pixels.data() + row * static_cast<std::size_t>(canvas.width()),
                    source_pitch);
    }
    SDL_UnlockTexture(texture);
    const SDL_FRect destination{0.0F, 0.0F, static_cast<float>(width),
                                static_cast<float>(height)};
    if (!SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g,
                                background_color.b, background_color.a) ||
        !SDL_RenderClear(renderer) ||
        !SDL_RenderTexture(renderer, texture, nullptr, &destination)) {
        return false;
    }

    return SDL_RenderPresent(renderer);
}


[[nodiscard]] bool map_key_code(SDL_Scancode scancode, storm::platform::KeyCode& output) noexcept {
    using storm::platform::KeyCode;
    switch (scancode) {
    case SDL_SCANCODE_A: output = KeyCode::A; return true;
    case SDL_SCANCODE_B: output = KeyCode::B; return true;
    case SDL_SCANCODE_C: output = KeyCode::C; return true;
    case SDL_SCANCODE_D: output = KeyCode::D; return true;
    case SDL_SCANCODE_E: output = KeyCode::E; return true;
    case SDL_SCANCODE_F: output = KeyCode::F; return true;
    case SDL_SCANCODE_G: output = KeyCode::G; return true;
    case SDL_SCANCODE_H: output = KeyCode::H; return true;
    case SDL_SCANCODE_I: output = KeyCode::I; return true;
    case SDL_SCANCODE_J: output = KeyCode::J; return true;
    case SDL_SCANCODE_K: output = KeyCode::K; return true;
    case SDL_SCANCODE_L: output = KeyCode::L; return true;
    case SDL_SCANCODE_M: output = KeyCode::M; return true;
    case SDL_SCANCODE_N: output = KeyCode::N; return true;
    case SDL_SCANCODE_O: output = KeyCode::O; return true;
    case SDL_SCANCODE_P: output = KeyCode::P; return true;
    case SDL_SCANCODE_Q: output = KeyCode::Q; return true;
    case SDL_SCANCODE_R: output = KeyCode::R; return true;
    case SDL_SCANCODE_S: output = KeyCode::S; return true;
    case SDL_SCANCODE_T: output = KeyCode::T; return true;
    case SDL_SCANCODE_U: output = KeyCode::U; return true;
    case SDL_SCANCODE_V: output = KeyCode::V; return true;
    case SDL_SCANCODE_W: output = KeyCode::W; return true;
    case SDL_SCANCODE_X: output = KeyCode::X; return true;
    case SDL_SCANCODE_Y: output = KeyCode::Y; return true;
    case SDL_SCANCODE_Z: output = KeyCode::Z; return true;
    case SDL_SCANCODE_0: output = KeyCode::Digit0; return true;
    case SDL_SCANCODE_1: output = KeyCode::Digit1; return true;
    case SDL_SCANCODE_2: output = KeyCode::Digit2; return true;
    case SDL_SCANCODE_3: output = KeyCode::Digit3; return true;
    case SDL_SCANCODE_4: output = KeyCode::Digit4; return true;
    case SDL_SCANCODE_5: output = KeyCode::Digit5; return true;
    case SDL_SCANCODE_6: output = KeyCode::Digit6; return true;
    case SDL_SCANCODE_7: output = KeyCode::Digit7; return true;
    case SDL_SCANCODE_8: output = KeyCode::Digit8; return true;
    case SDL_SCANCODE_9: output = KeyCode::Digit9; return true;
    case SDL_SCANCODE_ESCAPE: output = KeyCode::Escape; return true;
    case SDL_SCANCODE_RETURN: output = KeyCode::Enter; return true;
    case SDL_SCANCODE_TAB: output = KeyCode::Tab; return true;
    case SDL_SCANCODE_SPACE: output = KeyCode::Space; return true;
    case SDL_SCANCODE_BACKSPACE: output = KeyCode::Backspace; return true;
    case SDL_SCANCODE_DELETE: output = KeyCode::Delete; return true;
    case SDL_SCANCODE_INSERT: output = KeyCode::Insert; return true;
    case SDL_SCANCODE_HOME: output = KeyCode::Home; return true;
    case SDL_SCANCODE_END: output = KeyCode::End; return true;
    case SDL_SCANCODE_PAGEUP: output = KeyCode::PageUp; return true;
    case SDL_SCANCODE_PAGEDOWN: output = KeyCode::PageDown; return true;
    case SDL_SCANCODE_LEFT: output = KeyCode::ArrowLeft; return true;
    case SDL_SCANCODE_RIGHT: output = KeyCode::ArrowRight; return true;
    case SDL_SCANCODE_UP: output = KeyCode::ArrowUp; return true;
    case SDL_SCANCODE_DOWN: output = KeyCode::ArrowDown; return true;
    case SDL_SCANCODE_F1: output = KeyCode::F1; return true;
    case SDL_SCANCODE_F2: output = KeyCode::F2; return true;
    case SDL_SCANCODE_F3: output = KeyCode::F3; return true;
    case SDL_SCANCODE_F4: output = KeyCode::F4; return true;
    case SDL_SCANCODE_F5: output = KeyCode::F5; return true;
    case SDL_SCANCODE_F6: output = KeyCode::F6; return true;
    case SDL_SCANCODE_F7: output = KeyCode::F7; return true;
    case SDL_SCANCODE_F8: output = KeyCode::F8; return true;
    case SDL_SCANCODE_F9: output = KeyCode::F9; return true;
    case SDL_SCANCODE_F10: output = KeyCode::F10; return true;
    case SDL_SCANCODE_F11: output = KeyCode::F11; return true;
    case SDL_SCANCODE_F12: output = KeyCode::F12; return true;
    case SDL_SCANCODE_LSHIFT: output = KeyCode::LeftShift; return true;
    case SDL_SCANCODE_RSHIFT: output = KeyCode::RightShift; return true;
    case SDL_SCANCODE_LCTRL: output = KeyCode::LeftControl; return true;
    case SDL_SCANCODE_RCTRL: output = KeyCode::RightControl; return true;
    case SDL_SCANCODE_LALT: output = KeyCode::LeftAlt; return true;
    case SDL_SCANCODE_RALT: output = KeyCode::RightAlt; return true;
    default: return false;
    }
}

void print_sdl_error(const char* operation) {
    std::cerr << operation << " failed: " << SDL_GetError() << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Scene scene{Dimension::Two};
    storm::core::StableId player_id;
    const auto entity_status = scene.create_entity("Player", player_id);
    if (!entity_status.ok()) {
        std::cerr << "could not create the demo player entity: "
                  << entity_status.message() << '\n';
        return 1;
    }
    auto* player_entity = scene.find_entity(player_id);
    if (player_entity == nullptr) {
        std::cerr << "created demo player entity could not be found\n";
        return 1;
    }
    const auto renderable_status = player_entity->set_renderable2d(Renderable2D{});
    if (!renderable_status.ok()) {
        std::cerr << "could not attach the demo player's Renderable2D component: "
                  << renderable_status.message() << '\n';
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        print_sdl_error("SDL_Init");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "StormEngine 2D Bootstrap — WASD / arrows to move, wheel to zoom, click to move",
        window_initial_width, window_initial_height, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        print_sdl_error("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        print_sdl_error("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int window_width = window_initial_width;
    int window_height = window_initial_height;
    if (!SDL_GetWindowSize(window, &window_width, &window_height)) {
        print_sdl_error("SDL_GetWindowSize");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    InputState input;
    PlatformEventQueue<> event_queue;
    FixedStepClock simulation_clock;
    SoftwareCanvas2D canvas;
    SDL_Texture* texture = nullptr;

    Camera2D camera;
    camera.view_height = camera_initial_view_height;
    camera.viewport_width = static_cast<std::uint32_t>(std::max(window_width, 0));
    camera.viewport_height = static_cast<std::uint32_t>(std::max(window_height, 0));

    float player_size = player_initial_size;
    Vec2 player_position{0.0F, 0.0F};
    float cursor_x = 0.0F;
    float cursor_y = 0.0F;
    bool running = true;
    int exit_code = 0;
    std::uint64_t previous_time_ns = SDL_GetTicksNS();

    while (running) {
        input.begin_frame();
        const std::uint64_t dropped_before_poll = event_queue.dropped_count();

        SDL_Event native_event{};
        while (SDL_PollEvent(&native_event)) {
            switch (native_event.type) {
            case SDL_EVENT_QUIT:
                (void)enqueue(event_queue, PlatformEvent{storm::platform::QuitRequestedEvent{}});
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (native_event.window.windowID == SDL_GetWindowID(window)) {
                    (void)enqueue(
                        event_queue, PlatformEvent{storm::platform::QuitRequestedEvent{}});
                }
                break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (native_event.window.windowID == SDL_GetWindowID(window)) {
                    const bool focused = native_event.type == SDL_EVENT_WINDOW_FOCUS_GAINED;
                    (void)enqueue(
                        event_queue, PlatformEvent{storm::platform::WindowFocusChangedEvent{
                                         focused}});
                }
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                if (native_event.key.windowID != SDL_GetWindowID(window)) {
                    break;
                }
                storm::platform::KeyCode key{};
                if (map_key_code(native_event.key.scancode, key)) {
                    const bool is_down = native_event.type == SDL_EVENT_KEY_DOWN;
                    (void)enqueue(
                        event_queue,
                        PlatformEvent{storm::platform::KeyStateChangedEvent{key, is_down}});
                }
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (native_event.button.windowID == SDL_GetWindowID(window) &&
                    static_cast<std::size_t>(native_event.button.button) <
                        InputState::pointer_button_count) {
                    const bool is_down = native_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                    (void)enqueue(
                        event_queue,
                        PlatformEvent{storm::platform::PointerButtonStateChangedEvent{
                            static_cast<std::uint16_t>(native_event.button.button), is_down,
                            true, static_cast<double>(native_event.button.x),
                            static_cast<double>(native_event.button.y)}});
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (native_event.motion.windowID == SDL_GetWindowID(window)) {
                    (void)enqueue(
                        event_queue,
                        PlatformEvent{storm::platform::PointerMovedEvent{
                            static_cast<double>(native_event.motion.x),
                            static_cast<double>(native_event.motion.y)}});
                }
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                if (native_event.wheel.windowID == SDL_GetWindowID(window)) {
                    (void)enqueue(
                        event_queue,
                        PlatformEvent{storm::platform::PointerWheelScrolledEvent{
                            static_cast<double>(native_event.wheel.x),
                            static_cast<double>(native_event.wheel.y), true,
                            static_cast<double>(native_event.wheel.mouse_x),
                            static_cast<double>(native_event.wheel.mouse_y)}});
                }
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                if (native_event.window.windowID == SDL_GetWindowID(window)) {
                    const int width = std::max(native_event.window.data1, 0);
                    const int height = std::max(native_event.window.data2, 0);
                    (void)enqueue(
                        event_queue,
                        PlatformEvent{storm::platform::WindowResizedEvent{
                            static_cast<std::uint32_t>(width),
                            static_cast<std::uint32_t>(height)}});
                }
                break;

            default:
                break;
            }
        }

        while (const auto event = event_queue.pop()) {
            if (std::holds_alternative<storm::platform::QuitRequestedEvent>(*event)) {
                running = false;
            } else if (const auto* focus =
                           std::get_if<storm::platform::WindowFocusChangedEvent>(&*event)) {
                if (!focus->focused) {
                    input.release_all();
                }
            } else if (const auto* key =
                           std::get_if<storm::platform::KeyStateChangedEvent>(&*event)) {
                (void)input.set_key_down(key->key, key->is_down);
            } else if (const auto* button =
                           std::get_if<storm::platform::PointerButtonStateChangedEvent>(&*event)) {
                (void)input.set_pointer_button_down(button->button, button->is_down);
                if (button->has_position) {
                    cursor_x = static_cast<float>(button->x);
                    cursor_y = static_cast<float>(button->y);
                }
                if (button->is_down && button->button == SDL_BUTTON_LEFT) {
                    const auto clicked_world = camera.pixel_to_world(Vec2{cursor_x, cursor_y});
                    if (clicked_world.has_value()) {
                        player_position = *clicked_world;
                    }
                }
            } else if (const auto* pointer =
                           std::get_if<storm::platform::PointerMovedEvent>(&*event)) {
                cursor_x = static_cast<float>(pointer->x);
                cursor_y = static_cast<float>(pointer->y);
            } else if (const auto* wheel =
                           std::get_if<storm::platform::PointerWheelScrolledEvent>(&*event)) {
                if (wheel->has_position) {
                    cursor_x = static_cast<float>(wheel->x);
                    cursor_y = static_cast<float>(wheel->y);
                }

                float target_zoom = camera.zoom;
                if (wheel->delta_y > 0.0) {
                    target_zoom = std::min(camera.zoom * 1.1F, camera_maximum_zoom);
                } else if (wheel->delta_y < 0.0) {
                    target_zoom = std::max(camera.zoom / 1.1F, camera_minimum_zoom);
                }
                if (target_zoom != camera.zoom) {
                    (void)camera.set_zoom_at_pixel(target_zoom, Vec2{cursor_x, cursor_y});
                }
            } else if (const auto* resized =
                           std::get_if<storm::platform::WindowResizedEvent>(&*event)) {
                window_width = static_cast<int>(resized->width);
                window_height = static_cast<int>(resized->height);
                camera.viewport_width = resized->width;
                camera.viewport_height = resized->height;
            }
        }

        // A key-up or focus-loss event may have been dropped during an input burst. Release held
        // state after draining the accepted events instead of leaving stuck controls behind.
        if (event_queue.dropped_count() != dropped_before_poll) {
            input.release_all();
        }

        if (input.key_was_pressed(storm::platform::KeyCode::Escape)) {
            running = false;
        }
        if (!running) {
            break;
        }

        const std::uint64_t current_time_ns = SDL_GetTicksNS();
        const double frame_delta =
            current_time_ns >= previous_time_ns
                ? static_cast<double>(current_time_ns - previous_time_ns) /
                      nanoseconds_per_second
                : 0.0;
        previous_time_ns = current_time_ns;

        const auto advance = simulation_clock.advance(frame_delta);
        for (std::uint32_t step = 0U; step < advance.steps; ++step) {
            const bool left = input.key_is_down(storm::platform::KeyCode::A) ||
                              input.key_is_down(storm::platform::KeyCode::ArrowLeft);
            const bool right = input.key_is_down(storm::platform::KeyCode::D) ||
                               input.key_is_down(storm::platform::KeyCode::ArrowRight);
            const bool up = input.key_is_down(storm::platform::KeyCode::W) ||
                            input.key_is_down(storm::platform::KeyCode::ArrowUp);
            const bool down = input.key_is_down(storm::platform::KeyCode::S) ||
                              input.key_is_down(storm::platform::KeyCode::ArrowDown);
            const float horizontal = (right ? 1.0F : 0.0F) - (left ? 1.0F : 0.0F);
            const float vertical = (up ? 1.0F : 0.0F) - (down ? 1.0F : 0.0F);
            float direction_length = std::sqrt(horizontal * horizontal + vertical * vertical);
            const float distance = player_speed_units_per_second *
                                   static_cast<float>(
                                       simulation_clock.config().fixed_delta_seconds);
            if (direction_length > 0.0F) {
                direction_length = 1.0F / direction_length;
                player_position[0U] += horizontal * direction_length * distance;
                player_position[1U] += vertical * direction_length * distance;
            }
        }

        if (camera.viewport_width > 0U && camera.viewport_height > 0U) {
            const float half_view_height =
                camera.view_height / (2.0F * camera.zoom);
            const float aspect_ratio = static_cast<float>(camera.viewport_width) /
                                       static_cast<float>(camera.viewport_height);
            const float half_view_width = half_view_height * aspect_ratio;
            const float allowed_x =
                std::max(0.0F, half_view_width - player_size * 0.5F);
            const float allowed_y =
                std::max(0.0F, half_view_height - player_size * 0.5F);
            player_position[0U] =
                std::clamp(player_position[0U], camera.center[0U] - allowed_x,
                           camera.center[0U] + allowed_x);
            player_position[1U] =
                std::clamp(player_position[1U], camera.center[1U] - allowed_y,
                           camera.center[1U] + allowed_y);
        }

        const Transform2D player_transform{
            player_position, 0.0F, Vec2{player_size, player_size}};
        const auto transform_status = player_entity->set_transform(Transform{player_transform});
        if (!transform_status.ok()) {
            std::cerr << "could not update player transform: "
                      << transform_status.message() << '\n';
            exit_code = 1;
            break;
        }

        if (!ensure_software_surface(renderer, canvas, texture, window_width, window_height) ||
            !render_frame(renderer, texture, canvas, camera, scene,
                          window_width, window_height)) {
            print_sdl_error("render frame");
            exit_code = 1;
            break;
        }

        SDL_Delay(1U);
    }

    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}
