#include <cassert>
#include <cmath>

#include "storm/core/World.hpp"
#include "storm/render/Camera3D.hpp"
#include "storm/render/Renderable.hpp"
#include "storm/render/SceneRenderer.hpp"

using namespace storm;

int main() {
    core::World world;
    render::Camera3D camera;
    camera.setPosition({0.0f, 0.0f, 5.0f});
    camera.setViewport(1280.0f, 720.0f);

    const auto entity = world.createEntity();
    world.addTransform(entity).position = {0.0f, 0.0f, 0.0f};

    render::MeshDesc meshDesc{};
    meshDesc.vertexBuffer = render::BufferHandle(1, 1);
    meshDesc.vertexCount = 3;
    meshDesc.vertexLayout.attributeCount = 1;
    meshDesc.vertexLayout.stride = sizeof(float) * 3;
    meshDesc.vertexLayout.attributes[0] = {0, render::VertexFormat::Float32x3, 0};

    render::Material material(render::MaterialHandle(1, 1), render::ShaderHandle(2, 1));
    render::Renderable renderable{render::Mesh(meshDesc), material};
    world.registry().emplace<render::Renderable>(entity, renderable);

    render::RenderQueue queue;
    render::SceneRenderer renderer;
    renderer.build(world, camera, queue);

    assert(renderer.submittedCount() == 1);
    assert(renderer.rejectedCount() == 0);
    assert(queue.size() == 1);
    assert(queue.at(0).materialData != nullptr);
    assert(queue.at(0).materialData->hasParameter("uMVP"));

    const auto* mvp = std::get_if<render::MaterialMat4>(
        queue.at(0).materialData->parameter("uMVP"));
    assert(mvp != nullptr);
    assert(std::isfinite((*mvp)[0]));
    assert(std::isfinite((*mvp)[15]));

    world.getTransform(entity).position.x = 2.0f;
    renderer.build(world, camera, queue);
    assert(renderer.submittedCount() == 1);
    assert(queue.size() == 1);

    render::Renderable invalid = renderable;
    invalid.visible = false;
    world.registry().get<render::Renderable>(entity) = invalid;
    renderer.build(world, camera, queue);
    assert(renderer.submittedCount() == 0);
    assert(renderer.rejectedCount() == 1);
    assert(queue.empty());

    return 0;
}
