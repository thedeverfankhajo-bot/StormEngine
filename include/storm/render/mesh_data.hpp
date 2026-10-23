// Copyright 2026 StormEngine contributors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "storm/core/status.hpp"
#include "storm/math/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace storm::render {

// CPU-side vertex data for an indexed 3D triangle list. Values are kept in mesh-local space.
// Normals are validated as finite but are not implicitly normalized; UVs are stored unchanged.
struct MeshVertex final {
    math::Vec3 position{};
    math::Vec3 normal{};
    math::Vec2 texcoord{};

    [[nodiscard]] friend constexpr bool operator==(const MeshVertex&, const MeshVertex&) noexcept =
        default;
};

// Owns bounded CPU geometry. This is a validated data container, not a GPU buffer or renderable.
class MeshData final {
public:
    static constexpr std::size_t maximum_vertices = 1'000'000U;
    static constexpr std::size_t maximum_indices = 3'000'000U;

    // Replaces geometry only when a complete indexed triangle list is valid. Input is copied;
    // failed validation or allocation leaves the previous mesh unchanged.
    [[nodiscard]] core::Status set_geometry(std::span<const MeshVertex> vertices,
                                            std::span<const std::uint32_t> indices);

    // Releases all owned geometry.
    void reset() noexcept;

    [[nodiscard]] bool empty() const noexcept { return vertices_.empty() && indices_.empty(); }
    [[nodiscard]] std::size_t vertex_count() const noexcept { return vertices_.size(); }
    [[nodiscard]] std::size_t index_count() const noexcept { return indices_.size(); }
    [[nodiscard]] std::size_t triangle_count() const noexcept { return indices_.size() / 3U; }

    [[nodiscard]] std::span<const MeshVertex> vertices() const noexcept {
        return std::span<const MeshVertex>{vertices_.data(), vertices_.size()};
    }

    [[nodiscard]] std::span<const std::uint32_t> indices() const noexcept {
        return std::span<const std::uint32_t>{indices_.data(), indices_.size()};
    }

private:
    std::vector<MeshVertex> vertices_{};
    std::vector<std::uint32_t> indices_{};
};

} // namespace storm::render
