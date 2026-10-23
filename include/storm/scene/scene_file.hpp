#pragma once

#include "storm/scene/scene.hpp"

#include <filesystem>

namespace storm::scene {

// Bounded filesystem adapter for SceneCodec. Loading refuses a non-empty destination. Saving
// writes a secure temporary sibling, flushes it, then atomically replaces the target where the
// host filesystem supports that guarantee.
class SceneFile final {
public:
    [[nodiscard]] static core::Status save_atomic(const std::filesystem::path& destination,
                                                  const Scene& scene);

    [[nodiscard]] static core::Status load(const std::filesystem::path& source,
                                           Scene& destination);
};

} // namespace storm::scene
