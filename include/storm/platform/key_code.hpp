#pragma once
#include <cstddef>
#include <cstdint>

namespace storm::platform {

// Stable engine-level names for common physical keyboard keys. Letter and digit names describe
// physical key positions, not translated text; text input and IME belong to a separate service.
// Platform adapters translate native scancodes into this engine-owned enum.
enum class KeyCode : std::uint16_t {
    A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
    Escape, Enter, Tab, Space, Backspace, Delete, Insert, Home, End, PageUp, PageDown,
    ArrowLeft, ArrowRight, ArrowUp, ArrowDown,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt,
    Count
};

inline constexpr std::size_t key_code_count = static_cast<std::size_t>(KeyCode::Count);

[[nodiscard]] constexpr bool is_valid_key_code(KeyCode key) noexcept {
    return static_cast<std::size_t>(key) < key_code_count;
}

} // namespace storm::platform
