#include "storm/render/scene_renderer2d.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

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

[[nodiscard]] bool has_finite_quad(const math::Matrix4& transform,
                                   const math::Vec2& size) noexcept {
    if (!transform.is_finite() || !size.is_finite() ||
        size[0U] <= 0.0F || size[1U] <= 0.0F) {
        return false;
    }
    const std::array<math::Vec3, 4U> quad{
        math::Vec3{0.0F, 0.0F, 0.0F},
        math::Vec3{size[0U], 0.0F, 0.0F},
        math::Vec3{size[0U], size[1U], 0.0F},
        math::Vec3{0.0F, size[1U], 0.0F}
    };
    for (const math::Vec3& corner : quad) {
        if (!transform.transform_point(corner).has_value()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::uint8_t multiply_channel(std::uint8_t color,
                                            std::uint8_t tint) noexcept {
    const unsigned int product =
        static_cast<unsigned int>(color) * static_cast<unsigned int>(tint);
    return static_cast<std::uint8_t>((product + 127U) / 255U);
}

[[nodiscard]] Rgba8 apply_tint(Rgba8 base,
                               const std::array<std::uint8_t, 4U>& tint) noexcept {
    return Rgba8{multiply_channel(base.r, tint[0U]),
                 multiply_channel(base.g, tint[1U]),
                 multiply_channel(base.b, tint[2U]),
                 multiply_channel(base.a, tint[3U])};
}

void validate_entity(void* opaque, const scene::Entity& entity) noexcept {
    auto& context = *static_cast<PreflightContext*>(opaque);
    if (!context.valid) {
        return;
    }
    const scene::Renderable2D* renderable = entity.renderable2d();
    if (renderable == nullptr || !renderable->visible) {
        return;
    }

    const auto local_to_world = entity.transform().to_matrix();
    if (!local_to_world.has_value()) {
        context.valid = false;
        return;
    }
    const math::Matrix4 local_to_pixel = context.world_to_pixel * *local_to_world;
    context.valid = has_finite_quad(local_to_pixel, renderable->size);
}

void draw_entity(void* opaque, const scene::Entity& entity) noexcept {
    auto& context = *static_cast<DrawContext*>(opaque);
    if (!context.succeeded) {
        return;
    }
    const scene::Renderable2D* renderable = entity.renderable2d();
    if (renderable == nullptr || !renderable->visible) {
        return;
    }

    try {
        const auto local_to_world = entity.transform().to_matrix();
        if (!local_to_world.has_value()) {
            context.succeeded = false;
            return;
        }
        const core::Status status = context.canvas->draw_rect(
            context.world_to_pixel * *local_to_world, renderable->size,
            apply_tint(context.entity_color, renderable->tint_rgba));
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

    // Validate every visible renderable and its transformed quad before touching the prior
    // framebuffer. Scene and canvas are single-owner-thread objects; the second traversal sees
    // the same entities.
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
