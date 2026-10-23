#pragma once

#include "storm/core/status.hpp"
#include "storm/render/camera2d.hpp"
#include "storm/render/software_canvas2d.hpp"
#include "storm/scene/scene.hpp"

namespace storm::render {

// Connects 2D Scene/Entity/Transform and explicit Renderable2D data to the CPU reference renderer.
// Logic-only entities and invisible renderables are skipped. Texture/material resources are not
// implemented yet; visible renderables are drawn as tinted local quads.
class SceneRenderer2D final {
public:
    [[nodiscard]] static core::Status render(
        const scene::Scene& scene, const Camera2D& camera, SoftwareCanvas2D& canvas,
        Rgba8 clear_color = Rgba8{13U, 19U, 34U, 255U},
        Rgba8 entity_color = Rgba8{72U, 194U, 164U, 255U});
};

} // namespace storm::render
