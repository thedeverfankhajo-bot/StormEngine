#pragma once
#include "storm/core/scene.hpp"
#include "storm/foundation/result.hpp"
#include "storm/math/transform2d.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Storm::Rendering {

struct TextureKey final {
    std::uint64_t id{0U};
    std::uint32_t generation{0U};
    [[nodiscard]] bool is_valid() const noexcept { return id != 0U && generation != 0U; }
    [[nodiscard]] friend bool operator==(const TextureKey&, const TextureKey&) noexcept = default;
};

// Normalized UVs use a top-left texture origin.
struct SpriteUvRect final {
    float min_u{0.0F}, min_v{0.0F}, max_u{1.0F}, max_v{1.0F};
    [[nodiscard]] bool is_valid() const noexcept;
};

struct SpriteColor final {
    std::uint8_t r{255U}, g{255U}, b{255U}, a{255U};
};

// Scene component; Transform2D remains a separate component.
// TextureKey is an opaque batching identifier, not a GPU resource handle.
struct Sprite2DComponent final {
    TextureKey texture{};
    Math::Vector2 size{1.0F, 1.0F};
    Math::Vector2 pivot{0.5F, 0.5F};
    SpriteUvRect uv{};
    SpriteColor color{};
    std::int32_t sorting_layer{0};
    std::int32_t sorting_order{0};
};

struct SpriteVertex final {
    Math::Vector2 position{};
    Math::Vector2 uv{};
    SpriteColor color{};
};

struct SpriteBatchRange final {
    TextureKey texture{};
    std::uint32_t first_index{0U};
    std::uint32_t index_count{0U};
    std::int32_t sorting_layer{0};
    std::int32_t sorting_order{0};
};

// Non-owning spans; invalidated by any mutation/rebuild/move/destruction.
struct SpriteBatchView final {
    std::span<const SpriteVertex> vertices;
    std::span<const std::uint32_t> indices;
    std::span<const SpriteBatchRange> ranges;
    std::size_t sprite_count{0U};
    std::uint64_t revision{0U};
};

struct SpriteBatchConfig final { std::size_t maximum_sprites{4096U}; };

// CPU scene-to-sprite submission. It does not create GPU resources, issue
// graphics API calls, or present a frame.
class SpriteBatch final {
public:
    static constexpr std::size_t maximum_supported_sprites = 65'536U;
    [[nodiscard]] static Result<SpriteBatch> create(SpriteBatchConfig config = {});

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;
    SpriteBatch(SpriteBatch&& other) noexcept;
    SpriteBatch& operator=(SpriteBatch&& other) noexcept;

    [[nodiscard]] Result<void> append(
        const Sprite2DComponent& sprite,
        const Math::Matrix3& world_transform);
    [[nodiscard]] Result<void> append_local(
        const Sprite2DComponent& sprite,
        const Math::Transform2D& local_transform);

    void clear() noexcept;
    [[nodiscard]] Result<SpriteBatchView> build();
    [[nodiscard]] Result<SpriteBatchView> view() const;
    [[nodiscard]] std::size_t pending_sprite_count() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept;

private:
    struct PendingSprite final {
        Sprite2DComponent sprite{};
        Math::Matrix3 world_transform{};
        std::uint64_t submission_order{0U};
    };
    explicit SpriteBatch(SpriteBatchConfig config);
    [[nodiscard]] Result<void> ensure_initialized() const;

    SpriteBatchConfig config_{};
    std::vector<PendingSprite> pending_;
    std::vector<SpriteVertex> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<SpriteBatchRange> ranges_;
    std::uint64_t next_submission_order_{0U};
    std::uint64_t revision_{0U};
    bool built_{false};
    bool initialized_{true};
};

class SpriteSceneCollector2D final {
public:
    // Replaces the batch contents. On failure no partial geometry is exposed.
    // The existing overload keeps positions in world space.
    [[nodiscard]] static Result<SpriteBatchView> build(
        const Scene& scene, SpriteBatch& batch);

    // Applies Camera2D view-projection after each entity's world transform.
    // Output positions are normalized device coordinates in [-1, 1].
    [[nodiscard]] static Result<SpriteBatchView> build(
        const Scene& scene, SpriteBatch& batch, const Math::Camera2D& camera);
};

} // namespace Storm::Rendering
