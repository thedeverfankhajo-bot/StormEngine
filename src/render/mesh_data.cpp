// Copyright 2026 StormEngine contributors
// SPDX-License-Identifier: Apache-2.0
#include "storm/render/mesh_data.hpp"

#include <new>
#include <string>

namespace storm::render {
namespace {

[[nodiscard]] core::Status invalid_mesh_status(const char* message) {
    return core::Status::failure(core::ErrorCode::InvalidArgument, message);
}

[[nodiscard]] core::Status exhausted_mesh_status() {
    return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
}

} // namespace

core::Status MeshData::set_geometry(std::span<const MeshVertex> vertices,
                                    std::span<const std::uint32_t> indices) {
    if (vertices.empty()) {
        return invalid_mesh_status("mesh must contain at least one vertex");
    }
    if (indices.size() < 3U || indices.size() % 3U != 0U) {
        return invalid_mesh_status("mesh indices must describe a non-empty triangle list");
    }
    if (vertices.size() > maximum_vertices || indices.size() > maximum_indices) {
        return exhausted_mesh_status();
    }

    for (const MeshVertex& vertex : vertices) {
        if (!vertex.position.is_finite() || !vertex.normal.is_finite() ||
            !vertex.texcoord.is_finite()) {
            return invalid_mesh_status("mesh vertex contains a non-finite attribute");
        }
    }
    for (const std::uint32_t index : indices) {
        if (static_cast<std::size_t>(index) >= vertices.size()) {
            return invalid_mesh_status("mesh index refers outside the vertex array");
        }
    }

    try {
        std::vector<MeshVertex> next_vertices(vertices.begin(), vertices.end());
        std::vector<std::uint32_t> next_indices(indices.begin(), indices.end());
        vertices_.swap(next_vertices);
        indices_.swap(next_indices);
    } catch (const std::bad_alloc&) {
        return exhausted_mesh_status();
    }

    return core::Status::success();
}

void MeshData::reset() noexcept {
    std::vector<MeshVertex>{}.swap(vertices_);
    std::vector<std::uint32_t>{}.swap(indices_);
}

} // namespace storm::render
