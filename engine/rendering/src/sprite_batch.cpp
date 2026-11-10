#include "storm/rendering/sprite_batch.hpp"
#include "storm/core/transform_system_2d.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace Storm::Rendering {
namespace {
[[nodiscard]] Error invalid_argument(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}
[[nodiscard]] Error exhausted(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}
[[nodiscard]] bool valid_sprite(
    const Sprite2DComponent& sprite, const Math::Matrix3& world) noexcept {
    if (!sprite.texture.is_valid() || !sprite.size.is_finite() ||
        sprite.size.x <= 0.0F || sprite.size.y <= 0.0F ||
        !sprite.pivot.is_finite() || !sprite.uv.is_valid() ||
        !world.is_finite()) {
        return false;
    }
    constexpr float epsilon = 1.0e-6F;
    return std::fabs(world.at(2U, 0U)) <= epsilon &&
           std::fabs(world.at(2U, 1U)) <= epsilon &&
           std::fabs(world.at(2U, 2U) - 1.0F) <= epsilon;
}
[[nodiscard]] bool same_batch_key(
    const SpriteBatchRange& range, const Sprite2DComponent& sprite) noexcept {
    return range.texture == sprite.texture &&
           range.sorting_layer == sprite.sorting_layer &&
           range.sorting_order == sprite.sorting_order;
}
} // namespace

bool SpriteUvRect::is_valid() const noexcept {
    return std::isfinite(min_u) && std::isfinite(min_v) &&
           std::isfinite(max_u) && std::isfinite(max_v) &&
           min_u >= 0.0F && min_v >= 0.0F &&
           max_u <= 1.0F && max_v <= 1.0F &&
           min_u < max_u && min_v < max_v;
}

SpriteBatch::SpriteBatch(SpriteBatchConfig config) : config_(config) {
    pending_.reserve(config_.maximum_sprites);
    vertices_.reserve(config_.maximum_sprites * 4U);
    indices_.reserve(config_.maximum_sprites * 6U);
    ranges_.reserve(config_.maximum_sprites);
}

SpriteBatch::SpriteBatch(SpriteBatch&& other) noexcept
    : config_(other.config_), pending_(std::move(other.pending_)),
      vertices_(std::move(other.vertices_)), indices_(std::move(other.indices_)),
      ranges_(std::move(other.ranges_)),
      next_submission_order_(other.next_submission_order_),
      revision_(other.revision_), built_(other.built_),
      initialized_(std::exchange(other.initialized_, false)) {
    other.built_ = false;
    other.next_submission_order_ = 0U;
}

SpriteBatch& SpriteBatch::operator=(SpriteBatch&& other) noexcept {
    if (this != &other) {
        config_ = other.config_;
        pending_ = std::move(other.pending_);
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        ranges_ = std::move(other.ranges_);
        next_submission_order_ = other.next_submission_order_;
        revision_ = other.revision_;
        built_ = other.built_;
        initialized_ = std::exchange(other.initialized_, false);
        other.built_ = false;
        other.next_submission_order_ = 0U;
    }
    return *this;
}

Result<SpriteBatch> SpriteBatch::create(SpriteBatchConfig config) {
    if (config.maximum_sprites == 0U ||
        config.maximum_sprites > maximum_supported_sprites ||
        config.maximum_sprites >
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max() / 6U)) {
        return Result<SpriteBatch>::failure(invalid_argument(
            "SpriteBatch maximum_sprites is outside the supported range"));
    }
    try {
        return Result<SpriteBatch>::success(SpriteBatch{config});
    } catch (const std::bad_alloc&) {
        return Result<SpriteBatch>::failure(exhausted(
            "Unable to reserve bounded SpriteBatch storage"));
    } catch (const std::length_error&) {
        return Result<SpriteBatch>::failure(exhausted(
            "SpriteBatch capacity exceeds container limits"));
    }
}

Result<void> SpriteBatch::ensure_initialized() const {
    if (!initialized_) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "A moved-from SpriteBatch cannot be used"));
    }
    return Result<void>::success();
}

Result<void> SpriteBatch::append(
    const Sprite2DComponent& sprite, const Math::Matrix3& world_transform) {
    const auto initialized = ensure_initialized();
    if (!initialized) return initialized;
    if (!valid_sprite(sprite, world_transform)) {
        return Result<void>::failure(invalid_argument(
            "Sprite2D data or its world transform is invalid"));
    }
    if (pending_.size() >= config_.maximum_sprites) {
        return Result<void>::failure(exhausted(
            "SpriteBatch sprite capacity is exhausted"));
    }
    if (next_submission_order_ == std::numeric_limits<std::uint64_t>::max()) {
        return Result<void>::failure(exhausted(
            "SpriteBatch submission-order counter is exhausted"));
    }
    pending_.push_back(PendingSprite{
        .sprite = sprite,
        .world_transform = world_transform,
        .submission_order = next_submission_order_});
    ++next_submission_order_;
    built_ = false;
    return Result<void>::success();
}

Result<void> SpriteBatch::append_local(
    const Sprite2DComponent& sprite, const Math::Transform2D& local_transform) {
    const auto matrix = local_transform.to_matrix();
    if (!matrix) return Result<void>::failure(matrix.error());
    return append(sprite, matrix.value());
}

void SpriteBatch::clear() noexcept {
    if (!initialized_) return;
    pending_.clear();
    vertices_.clear();
    indices_.clear();
    ranges_.clear();
    next_submission_order_ = 0U;
    built_ = false;
}

Result<SpriteBatchView> SpriteBatch::build() {
    const auto initialized = ensure_initialized();
    if (!initialized) return Result<SpriteBatchView>::failure(initialized.error());
    if (revision_ == std::numeric_limits<std::uint64_t>::max()) {
        return Result<SpriteBatchView>::failure(exhausted(
            "SpriteBatch output revision counter is exhausted"));
    }

    vertices_.clear();
    indices_.clear();
    ranges_.clear();
    built_ = false;
    std::sort(pending_.begin(), pending_.end(),
        [](const PendingSprite& lhs, const PendingSprite& rhs) {
            return std::tie(lhs.sprite.sorting_layer, lhs.sprite.sorting_order,
                            lhs.submission_order) <
                   std::tie(rhs.sprite.sorting_layer, rhs.sprite.sorting_order,
                            rhs.submission_order);
        });

    constexpr std::array<std::uint32_t, 6U> quad_indices{0U, 1U, 2U, 2U, 3U, 0U};
    for (const PendingSprite& pending : pending_) {
        const auto& sprite = pending.sprite;
        const float left = -sprite.pivot.x * sprite.size.x;
        const float bottom = -sprite.pivot.y * sprite.size.y;
        const float right = left + sprite.size.x;
        const float top = bottom + sprite.size.y;
        const std::array<Math::Vector2, 4U> local{{
            {left, bottom}, {right, bottom}, {right, top}, {left, top}}};
        // Positions are CCW (+Y up); UVs use a top-left image origin.
        const std::array<Math::Vector2, 4U> uvs{{
            {sprite.uv.min_u, sprite.uv.max_v},
            {sprite.uv.max_u, sprite.uv.max_v},
            {sprite.uv.max_u, sprite.uv.min_v},
            {sprite.uv.min_u, sprite.uv.min_v}}};

        std::array<Math::Vector2, 4U> world_positions{};
        for (std::size_t i = 0U; i < local.size(); ++i) {
            const auto transformed = pending.world_transform.transform_point(local[i]);
            if (!transformed) {
                vertices_.clear(); indices_.clear(); ranges_.clear();
                return Result<SpriteBatchView>::failure(transformed.error());
            }
            world_positions[i] = transformed.value();
        }

        const std::size_t base_size = vertices_.size();
        if (base_size > static_cast<std::size_t>(
                std::numeric_limits<std::uint32_t>::max() - 3U)) {
            vertices_.clear(); indices_.clear(); ranges_.clear();
            return Result<SpriteBatchView>::failure(exhausted(
                "SpriteBatch vertex indices exceed the 32-bit index range"));
        }
        const auto base_vertex = static_cast<std::uint32_t>(base_size);
        const auto first_index = static_cast<std::uint32_t>(indices_.size());
        for (std::size_t i = 0U; i < world_positions.size(); ++i) {
            vertices_.push_back(SpriteVertex{
                .position = world_positions[i], .uv = uvs[i], .color = sprite.color});
        }
        for (const auto index : quad_indices) indices_.push_back(base_vertex + index);

        if (!ranges_.empty() && same_batch_key(ranges_.back(), sprite)) {
            ranges_.back().index_count += 6U;
        } else {
            ranges_.push_back(SpriteBatchRange{
                .texture = sprite.texture, .first_index = first_index,
                .index_count = 6U, .sorting_layer = sprite.sorting_layer,
                .sorting_order = sprite.sorting_order});
        }
    }

    ++revision_;
    built_ = true;
    return Result<SpriteBatchView>::success(SpriteBatchView{
        .vertices = std::span<const SpriteVertex>{vertices_},
        .indices = std::span<const std::uint32_t>{indices_},
        .ranges = std::span<const SpriteBatchRange>{ranges_},
        .sprite_count = pending_.size(),
        .revision = revision_});
}

Result<SpriteBatchView> SpriteBatch::view() const {
    const auto initialized = ensure_initialized();
    if (!initialized) return Result<SpriteBatchView>::failure(initialized.error());
    if (!built_) {
        return Result<SpriteBatchView>::failure(Error::make(
            ErrorCode::InvalidState,
            "SpriteBatch::view requires a successful build after the last mutation"));
    }
    return Result<SpriteBatchView>::success(SpriteBatchView{
        .vertices = std::span<const SpriteVertex>{vertices_},
        .indices = std::span<const std::uint32_t>{indices_},
        .ranges = std::span<const SpriteBatchRange>{ranges_},
        .sprite_count = pending_.size(),
        .revision = revision_});
}

std::size_t SpriteBatch::pending_sprite_count() const noexcept {
    return initialized_ ? pending_.size() : 0U;
}
std::size_t SpriteBatch::capacity() const noexcept {
    return initialized_ ? config_.maximum_sprites : 0U;
}

namespace {
[[nodiscard]] Result<SpriteBatchView> collect_scene_sprites(
    const Scene& scene, SpriteBatch& batch,
    const Math::Matrix3* view_projection) {
    batch.clear();
    std::optional<Error> collection_error;
    try {
        const auto visited = scene.for_each_component<Sprite2DComponent>(
            [&scene, &batch, &collection_error, view_projection](
                EntityHandle entity, const Sprite2DComponent& sprite) {
                if (collection_error.has_value()) return;

                const auto world = TransformSystem2D::world_matrix(scene, entity);
                if (!world) {
                    collection_error = world.error();
                    return;
                }

                Math::Matrix3 sprite_transform = world.value();
                if (view_projection != nullptr) {
                    const auto camera_transform =
                        Math::Matrix3::multiply_checked(*view_projection, sprite_transform);
                    if (!camera_transform) {
                        collection_error = camera_transform.error();
                        return;
                    }
                    sprite_transform = camera_transform.value();
                }

                const auto appended = batch.append(sprite, sprite_transform);
                if (!appended) collection_error = appended.error();
            });

        if (!visited) {
            batch.clear();
            return Result<SpriteBatchView>::failure(visited.error());
        }
        if (collection_error.has_value()) {
            batch.clear();
            return Result<SpriteBatchView>::failure(std::move(*collection_error));
        }
        return batch.build();
    } catch (const std::bad_alloc&) {
        batch.clear();
        return Result<SpriteBatchView>::failure(exhausted(
            "Unable to collect Scene sprite submissions"));
    } catch (const std::length_error&) {
        batch.clear();
        return Result<SpriteBatchView>::failure(exhausted(
            "Scene sprite submission exceeded container limits"));
    }
}
} // namespace

Result<SpriteBatchView> SpriteSceneCollector2D::build(
    const Scene& scene, SpriteBatch& batch) {
    return collect_scene_sprites(scene, batch, nullptr);
}

Result<SpriteBatchView> SpriteSceneCollector2D::build(
    const Scene& scene, SpriteBatch& batch, const Math::Camera2D& camera) {
    const auto view_projection = camera.view_projection();
    if (!view_projection) {
        batch.clear();
        return Result<SpriteBatchView>::failure(view_projection.error());
    }
    return collect_scene_sprites(scene, batch, &view_projection.value());
}

Result<SpriteBatchView> SpriteSceneCollector2D::build_active_camera(
    const Scene& scene, SpriteBatch& batch) {
    const Camera2DComponent* selected_camera = nullptr;
    const auto cameras = scene.for_each_component<Camera2DComponent>(
        [&selected_camera](EntityHandle, const Camera2DComponent& candidate) {
            if (!candidate.enabled) return;
            if (selected_camera == nullptr ||
                candidate.priority > selected_camera->priority) {
                selected_camera = &candidate;
            }
        });

    if (!cameras) {
        batch.clear();
        return Result<SpriteBatchView>::failure(cameras.error());
    }
    if (selected_camera == nullptr) {
        batch.clear();
        return Result<SpriteBatchView>::failure(Error::make(
            ErrorCode::InvalidState,
            "Scene has no enabled Camera2DComponent"));
    }
    return build(scene, batch, selected_camera->camera);
}

} // namespace Storm::Rendering
