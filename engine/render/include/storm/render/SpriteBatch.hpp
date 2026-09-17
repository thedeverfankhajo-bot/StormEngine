#pragma once

#include "storm/math/Vec2.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace storm::render {

struct SpriteVertex final {
    math::Vec2 position{};
    math::Vec2 uv{};
};

struct SpriteBatchGeometry final {
    std::vector<SpriteVertex> vertices;
    std::vector<std::uint32_t> indices;

    [[nodiscard]] bool valid() const noexcept {
        if (vertices.empty() || indices.empty() || (indices.size() % 6u) != 0u) return false;
        for (const auto index : indices)
            if (index >= vertices.size()) return false;
        return true;
    }

    [[nodiscard]] std::size_t spriteCount() const noexcept { return indices.size() / 6u; }
};

class SpriteBatch final {
public:
    explicit SpriteBatch(std::size_t reserveSprites = 0) {
        if (reserveSprites > 0) reserve(reserveSprites);
    }

    void clear() noexcept {
        vertices_.clear();
        indices_.clear();
    }

    void reserve(std::size_t sprites) {
        if (sprites > std::numeric_limits<std::size_t>::max() / 4u ||
            sprites > std::numeric_limits<std::size_t>::max() / 6u) return;
        vertices_.reserve(sprites * 4u);
        indices_.reserve(sprites * 6u);
    }

    bool add(math::Vec2 position,
             math::Vec2 size,
             math::Vec2 uvMin = {0.0f, 0.0f},
             math::Vec2 uvMax = {1.0f, 1.0f}) {
        if (!finite(position) || !finite(size) || !finite(uvMin) || !finite(uvMax)) return false;
        if (vertices_.size() > std::numeric_limits<std::uint32_t>::max() - 4u) return false;
        const std::uint32_t base = static_cast<std::uint32_t>(vertices_.size());
        const math::Vec2 maxPosition = position + size;
        vertices_.push_back({position, {uvMin.x, uvMin.y}});
        vertices_.push_back({{maxPosition.x, position.y}, {uvMax.x, uvMin.y}});
        vertices_.push_back({maxPosition, uvMax});
        vertices_.push_back({{position.x, maxPosition.y}, {uvMin.x, uvMax.y}});
        indices_.insert(indices_.end(), {base, base + 1u, base + 2u,
                                         base, base + 2u, base + 3u});
        return true;
    }

    [[nodiscard]] SpriteBatchGeometry build() const { return {vertices_, indices_}; }
    [[nodiscard]] const std::vector<SpriteVertex>& vertices() const noexcept { return vertices_; }
    [[nodiscard]] const std::vector<std::uint32_t>& indices() const noexcept { return indices_; }
    [[nodiscard]] std::size_t spriteCount() const noexcept { return indices_.size() / 6u; }

private:
    static bool finite(math::Vec2 value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y);
    }

    std::vector<SpriteVertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

} // namespace storm::render
