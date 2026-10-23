#pragma once

#include "storm/core/status.hpp"
#include "storm/render/camera2d.hpp"
#include "storm/render/software_canvas2d.hpp"
#include "storm/scene/scene.hpp"

namespace storm::render {

// Connects the dimension-aware Scene/Entity/Transform model to the CPU reference renderer.
// Until renderable components/materials exist, each 2D entity is drawn as a local unit quad,
// with its Transform2D controlling world position, rotation and size.
class SceneRenderer2D final {
public:
    [[nodiscard]] static core::Status render(
        const scene::Scene& scene, const Camera2D& camera, SoftwareCanvas2D& canvas,
        Rgba8 clear_color = Rgba8{13U, 19U, 34U, 255U},
        Rgba8 entity_color = Rgba8{72U, 194U, 164U, 255U});
};

} // namespace storm::render
