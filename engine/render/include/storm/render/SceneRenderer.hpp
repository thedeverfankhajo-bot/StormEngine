#pragma once

#include "Camera3D.hpp"
#include "RenderQueue.hpp"
#include "Renderable.hpp"
#include "storm/core/World.hpp"
#include "storm/math/Mat4.hpp"
#include <array>
#include <deque>
#include <limits>
#include <cstddef>
#include <utility>
#include <vector>

namespace storm::render {

class SceneRenderer final {
public:
    explicit SceneRenderer(std::size_t materialReserve = 64) {
        materialCopies_.resize(0);
        materialReserve_ = materialReserve;
    }

    void reserve(std::size_t renderables) {
        materialReserve_ = renderables;
    }

    void beginFrame() noexcept {
        materialCopies_.clear();
        submittedCount_ = 0;
        rejectedCount_ = 0;
    }

    [[nodiscard]] std::size_t submittedCount() const noexcept { return submittedCount_; }
    [[nodiscard]] std::size_t rejectedCount() const noexcept { return rejectedCount_; }

    // Builds CPU-side draw commands from the ECS world. Material snapshots are
    // retained by this renderer until the next beginFrame(), so DrawCommand's
    // materialData pointer remains valid while the queue is executed.
    void build(core::World& world, const Camera3D& camera, RenderQueue& queue) {
        beginFrame();
        queue.beginFrame();

        world.registry().each<math::Transform, Renderable>(
            [&](ecs::Entity entity, math::Transform&, Renderable& renderable) {
                if (!renderable.valid()) {
                    ++rejectedCount_;
                    return;
                }

                const math::Mat4 model = world.worldMatrix(entity);
                const math::Mat4 mvp = camera.viewProjectionMatrix() * model;

                Material snapshot = renderable.material;
                snapshot.setParameter("uMVP", toMaterialMat4(mvp));

                if (camera.viewportWidth() > static_cast<float>(std::numeric_limits<std::uint32_t>::max()) ||
                    camera.viewportHeight() > static_cast<float>(std::numeric_limits<std::uint32_t>::max())) {
                    ++rejectedCount_;
                    return;
                }

                DrawCommand command = renderable.mesh.drawCommand();
                command.shader = snapshot.shader();
                command.fragmentShader = snapshot.shader();
                command.material = snapshot.handle();
                command.materialData = &materialCopies_.emplace_back(std::move(snapshot));
                command.viewportWidth = static_cast<std::uint32_t>(camera.viewportWidth());
                command.viewportHeight = static_cast<std::uint32_t>(camera.viewportHeight());

                if (command.shader.valid() && command.fragmentShader.valid() && queue.submit(command)) {
                    ++submittedCount_;
                } else {
                    ++rejectedCount_;
                    materialCopies_.pop_back();
                }
            });
    }

private:
    static MaterialMat4 toMaterialMat4(const math::Mat4& matrix) noexcept {
        MaterialMat4 result{};
        // OpenGL ES glUniformMatrix4fv(..., GL_FALSE, ...) consumes column-major
        // data. Mat4 is stored row-major, so explicitly transpose while flattening.
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t col = 0; col < 4; ++col)
                result[col * 4 + row] = matrix.m[row][col];
        return result;
    }

    std::deque<Material> materialCopies_;
    std::size_t materialReserve_{64};
    std::size_t submittedCount_{0};
    std::size_t rejectedCount_{0};
};

} // namespace storm::render
