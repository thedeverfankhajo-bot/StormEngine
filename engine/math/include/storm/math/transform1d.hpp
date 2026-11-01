#pragma once

#include <cmath>

namespace Storm::Math {

// A scalar gameplay transform; it never introduces hidden planar or spatial axes.
struct Transform1D final {
    double position{0.0};

    [[nodiscard]] bool is_valid() const noexcept {
        return std::isfinite(position);
    }
};

} // namespace Storm::Math
