#include "storm/render/scene_renderer2d.hpp"

#include <array>
#include <cstddef>

namespace storm::render {
namespace {

struct PreflightContext final {
    math::Matrix4 world_to_pixel{};
    bool valid{true};
};

struct DrawContext final {
    SoftwareCanvas2D* canvas{};
    math::Matrix4 world_to_pixel{};
    Rgba8 entity_color{};
    bool succeeded{true};
};

[[nodiscard]] bool has_finite_quad(const math::Matrix4& transform) noexcept {
    if (!transform.is_finite()) {
        return false;
    }

    constexpr std::array<math::Vec3, 4U> unit_quad{
        math::Vec3{0.0F, 0.0F, 0.0F},
        math::Vec3{1.0F, 0.0F, 0.0F},
        math::Vec3{1.0F, 1.0F, 0.0F},
        math::Vec3{0.0F, 1.0F, 0.0F}
    };
    for (const math::Vec3& corner : unit_quad) {
        if (!transform.transform_point(corner).has_value()) {
            return false;
        }
    }
    return true;
}

void validate_entity(void* opaque, const scene::Entity& entity) noexcept {
    auto& context = *static_cast<PreflightContext*>(opaque);
    if (!context.valid) {
        return;
    }

    const auto local_to_world = entity.transform().to_matrix();
    if (!local_to_world.has_value()) {
        context.valid = false;
        return;
    }

    const math::Matrix4 local_to_pixel = context.world_to_pixel * *local_to_world;
    context.valid = has_finite_quad(local_to_pixel);
}

void draw_entity(void* opaque, const scene::Entity& entity) noexcept {
    auto& context = *static_cast<DrawContext*>(opaque);
    if (!context.succeeded) {
        return;
    }

    try {
        const auto local_to_world = entity.transform().to_matrix();
        if (!local_to_world.has_value()) {
            context.succeeded = false;
            return;
        }

        const core::Status status = context.canvas->draw_rect(
            context.world_to_pixel * *local_to_world, math::Vec2{1.0F, 1.0F},
            context.entity_color);
        context.succeeded = status.ok();
    } catch (...) {
        // Scene visitors are explicitly noexcept. Convert unexpected exceptions into failure.
        context.succeeded = false;
    }
}

} // namespace

core::Status SceneRenderer2D::render(const scene::Scene& source_scene, const Camera2D& camera,
                                     SoftwareCanvas2D& canvas, Rgba8 clear_color,
                                     Rgba8 entity_color) {
    const core::Status scene_status = source_scene.validate();
    if (!scene_status.ok()) {
        return scene_status;
    }
    if (source_scene.dimension() != storm::scene::Dimension::Two) {
        return core::Status::failure(
            core::ErrorCode::Unsupported, "SceneRenderer2D only renders 2D scenes");
    }

    if (canvas.width() == 0U || canvas.height() == 0U ||
        camera.viewport_width != canvas.width() ||
        camera.viewport_height != canvas.height()) {
        return core::Status::failure(
            core::ErrorCode::InvalidArgument,
            "2D camera viewport must match a non-empty software canvas");
    }

    const auto world_to_pixel = camera.world_to_pixel_matrix();
    if (!world_to_pixel.has_value()) {
        return core::Status::failure(
            core::ErrorCode::InvalidArgument,
            "2D camera parameters are invalid or unrepresentable");
    }

    // Validate every transform and transformed unit quad before touching the prior framebuffer.
    // Scene and canvas are single-owner-thread objects; the second traversal sees the same entities.
    PreflightContext preflight{*world_to_pixel, true};
    const std::size_t validated = source_scene.visit_entities(&preflight, &validate_entity);
    if (!preflight.valid || validated != source_scene.entity_count()) {
        return core::Status::failure(
            core::ErrorCode::InvalidArgument,
            "scene contains a transform that cannot be represented by the 2D renderer");
    }

    canvas.clear(clear_color);
    DrawContext drawing{&canvas, *world_to_pixel, entity_color, true};
    const std::size_t rendered = source_scene.visit_entities(&drawing, &draw_entity);
    if (!drawing.succeeded || rendered != source_scene.entity_count()) {
        return core::Status::failure(
            core::ErrorCode::Internal, "2D scene rendering failed after transform validation");
    }

    return core::Status::success();
}

} // namespace storm::render
