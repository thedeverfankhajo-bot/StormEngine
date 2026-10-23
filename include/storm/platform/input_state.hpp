#pragma once

#include "storm/platform/key_code.hpp"

#include <bitset>
#include <cstddef>

namespace storm::platform {

// Adapter-fed digital input snapshot. This class never polls the operating system and performs
// no dynamic allocation. The native platform layer is responsible for mapping device inputs to
// key slots [0, key_code_count) and pointer-button slots [0, pointer_button_count).
//
// Call begin_frame() once before processing input for a new frame. The type is intentionally
// single-owner-thread only; it does not synchronize concurrent event producers.
class InputState final {
public:
    static constexpr std::size_t key_code_count = 256U;
    static constexpr std::size_t pointer_button_count = 8U;

    // Clears only edge flags. Held inputs remain held across frames.
    void begin_frame() noexcept {
        keys_pressed_.reset();
        keys_released_.reset();
        pointer_buttons_pressed_.reset();
        pointer_buttons_released_.reset();
    }

    // Returns false for an unsupported slot and leaves all state unchanged.
    // Repeating the current state is accepted but does not create another edge.
    [[nodiscard]] bool set_key_down(std::size_t key_code, bool is_down) noexcept {
        return update_button(keys_down_, keys_pressed_, keys_released_, key_code, is_down);
    }

    [[nodiscard]] bool set_key_down(KeyCode key, bool is_down) noexcept {
        return is_valid_key_code(key) &&
               set_key_down(static_cast<std::size_t>(key), is_down);
    }

    [[nodiscard]] bool set_pointer_button_down(std::size_t button, bool is_down) noexcept {
        return update_button(pointer_buttons_down_, pointer_buttons_pressed_,
                             pointer_buttons_released_, button, is_down);
    }

    [[nodiscard]] bool key_is_down(std::size_t key_code) const noexcept {
        return is_set(keys_down_, key_code);
    }

    [[nodiscard]] bool key_was_pressed(std::size_t key_code) const noexcept {
        return is_set(keys_pressed_, key_code);
    }

    [[nodiscard]] bool key_was_released(std::size_t key_code) const noexcept {
        return is_set(keys_released_, key_code);
    }

    [[nodiscard]] bool key_is_down(KeyCode key) const noexcept {
        return is_valid_key_code(key) && key_is_down(static_cast<std::size_t>(key));
    }

    [[nodiscard]] bool key_was_pressed(KeyCode key) const noexcept {
        return is_valid_key_code(key) && key_was_pressed(static_cast<std::size_t>(key));
    }

    [[nodiscard]] bool key_was_released(KeyCode key) const noexcept {
        return is_valid_key_code(key) && key_was_released(static_cast<std::size_t>(key));
    }

    [[nodiscard]] bool pointer_button_is_down(std::size_t button) const noexcept {
        return is_set(pointer_buttons_down_, button);
    }

    [[nodiscard]] bool pointer_button_was_pressed(std::size_t button) const noexcept {
        return is_set(pointer_buttons_pressed_, button);
    }

    [[nodiscard]] bool pointer_button_was_released(std::size_t button) const noexcept {
        return is_set(pointer_buttons_released_, button);
    }

    // Call when focus is lost or an input device is detached. Held controls become released,
    // preventing keys/buttons from remaining stuck if their native release event is never sent.
    void release_all() noexcept {
        keys_released_ |= keys_down_;
        keys_down_.reset();
        pointer_buttons_released_ |= pointer_buttons_down_;
        pointer_buttons_down_.reset();
    }

    // Clears held and edge state. Unlike release_all(), this does not publish release edges.
    void reset() noexcept {
        keys_down_.reset();
        keys_pressed_.reset();
        keys_released_.reset();
        pointer_buttons_down_.reset();
        pointer_buttons_pressed_.reset();
        pointer_buttons_released_.reset();
    }

private:
    template <std::size_t Count>
    [[nodiscard]] static bool update_button(std::bitset<Count>& down,
                                            std::bitset<Count>& pressed,
                                            std::bitset<Count>& released,
                                            std::size_t code, bool is_down) noexcept {
        if (code >= Count) {
            return false;
        }

        const bool was_down = down.test(code);
        if (was_down == is_down) {
            return true;
        }

        down.set(code, is_down);
        if (is_down) {
            pressed.set(code);
        } else {
            released.set(code);
        }
        return true;
    }

    template <std::size_t Count>
    [[nodiscard]] static bool is_set(const std::bitset<Count>& state,
                                     std::size_t code) noexcept {
        return code < Count && state.test(code);
    }

    std::bitset<key_code_count> keys_down_{};
    std::bitset<key_code_count> keys_pressed_{};
    std::bitset<key_code_count> keys_released_{};
    std::bitset<pointer_button_count> pointer_buttons_down_{};
    std::bitset<pointer_button_count> pointer_buttons_pressed_{};
    std::bitset<pointer_button_count> pointer_buttons_released_{};
};

} // namespace storm::platform
