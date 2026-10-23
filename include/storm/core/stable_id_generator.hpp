#pragma once

#include "storm/core/stable_id.hpp"
#include "storm/core/status.hpp"

namespace storm::core {

// Generates RFC 9562 UUID version 4 values using the host operating system's secure random
// source. On failure, output is left unchanged and no weaker pseudo-random fallback is used.
class StableIdGenerator final {
public:
    [[nodiscard]] static Status generate_v4(StableId& output);
};

} // namespace storm::core
