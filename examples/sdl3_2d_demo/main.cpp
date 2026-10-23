#include "storm/core/fixed_step_clock.hpp"
#include "storm/platform/event_queue.hpp"
#include "storm/platform/input_state.hpp"
#include "storm/platform/key_code.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace {

using storm::core::FixedStepClock;
using storm::platform::InputState;
using storm::platform::PlatformEvent;
using storm::platform::PlatformEventQueue;

constexpr int window_initial_width = 960;
constexpr int window_initial_height = 540;
constexpr float player_initial_size = 48.0F;
constexpr float player_minimum_size = 20.0F;
constexpr float player_maximum_size = 112.0F;
constexpr float player_speed_units_per_second = 260.0F;
constexpr double nanoseconds_per_second = 1'000'000'000.0;

[[nodiscard]] bool enqueue(PlatformEventQueue<>& queue,
                           const PlatformEvent& event) noexcept {
    return queue.push(event);
}

[[nodiscard]] bool render_frame(SDL_Renderer* renderer, int width, int height,
                                float player_x, float player_y, float player_size) noexcept {
    if (!SDL_SetRenderDrawColor(renderer, 13U, 19U, 34U, 255U) ||
        !SDL_RenderClear(renderer)) {
        return false;
    }

    if (width > 0 && height > 0) {
        const SDL_FRect player{player_x, player_y, player_size, player_size};
        if (!SDL_SetRenderDrawColor(renderer, 72U, 194U, 164U, 255U) ||
            !SDL_RenderFillRect(renderer, &player)) {
            return false;
        }
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

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        print_sdl_error("SDL_Init");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "StormEngine 2D Bootstrap — WASD / arrows to move, mouse wheel to resize, Esc to quit",
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

    float player_size = player_initial_size;
    float player_x = (static_cast<float>(window_width) - player_size) * 0.5F;
    float player_y = (static_cast<float>(window_height) - player_size) * 0.5F;
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
                            static_cast<std::uint16_t>(native_event.button.button), is_down}});
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
                            static_cast<double>(native_event.wheel.y)}});
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
                (void)input.set_key_down(key->key_code, key->is_down);
            } else if (const auto* button =
                           std::get_if<storm::platform::PointerButtonStateChangedEvent>(&*event)) {
                (void)input.set_pointer_button_down(button->button, button->is_down);
                if (button->is_down && button->button == SDL_BUTTON_LEFT) {
                    player_x = cursor_x - player_size * 0.5F;
                    player_y = cursor_y - player_size * 0.5F;
                }
            } else if (const auto* pointer =
                           std::get_if<storm::platform::PointerMovedEvent>(&*event)) {
                cursor_x = static_cast<float>(pointer->x);
                cursor_y = static_cast<float>(pointer->y);
            } else if (const auto* wheel =
                           std::get_if<storm::platform::PointerWheelScrolledEvent>(&*event)) {
                if (wheel->delta_y > 0.0) {
                    player_size = std::min(player_size + 4.0F, player_maximum_size);
                } else if (wheel->delta_y < 0.0) {
                    player_size = std::max(player_size - 4.0F, player_minimum_size);
                }
            } else if (const auto* resized =
                           std::get_if<storm::platform::WindowResizedEvent>(&*event)) {
                window_width = static_cast<int>(resized->width);
                window_height = static_cast<int>(resized->height);
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
            const float vertical = (down ? 1.0F : 0.0F) - (up ? 1.0F : 0.0F);
            const float distance = player_speed_units_per_second *
                                   static_cast<float>(
                                       simulation_clock.config().fixed_delta_seconds);
            player_x += horizontal * distance;
            player_y += vertical * distance;
        }

        if (window_width > 0 && window_height > 0) {
            const float max_x =
                std::max(0.0F, static_cast<float>(window_width) - player_size);
            const float max_y =
                std::max(0.0F, static_cast<float>(window_height) - player_size);
            player_x = std::clamp(player_x, 0.0F, max_x);
            player_y = std::clamp(player_y, 0.0F, max_y);
        }

        if (!render_frame(renderer, window_width, window_height,
                          player_x, player_y, player_size)) {
            print_sdl_error("render frame");
            exit_code = 1;
            break;
        }

        SDL_Delay(1U);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}
