#include "storm/serialization/scene_serializer.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Storm::Serialization {

// Instantiate built-in scalar and spatial transform archive paths in every
// configured mobile target build. User-defined component codecs remain template-driven.
template Result<std::vector<std::byte>>
SceneSerializer::serialize<Math::Transform1D>(
    const Scene& scene,
    const SceneArchiveLimits& limits);

template Result<Scene>
SceneSerializer::deserialize<Math::Transform1D>(
    std::span<const std::byte> archive,
    const SceneArchiveLimits& limits);

template Result<std::vector<std::byte>>
SceneSerializer::serialize<Math::Transform3D>(
    const Scene& scene,
    const SceneArchiveLimits& limits);

template Result<Scene>
SceneSerializer::deserialize<Math::Transform3D>(
    std::span<const std::byte> archive,
    const SceneArchiveLimits& limits);

template Result<std::vector<std::byte>>
SceneSerializer::serialize<Math::Transform2D>(
    const Scene& scene, const SceneArchiveLimits& limits);
template Result<Scene>
SceneSerializer::deserialize<Math::Transform2D>(
    std::span<const std::byte> archive, const SceneArchiveLimits& limits);

} // namespace Storm::Serialization
