// Copyright 2026 StormEngine contributors
// SPDX-License-Identifier: Apache-2.0
#include "storm/render/mesh_data.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

[[nodiscard]] std::array<storm::render::MeshVertex, 3U> make_triangle() {
    using storm::math::Vec2;
    using storm::math::Vec3;
    using storm::render::MeshVertex;
    return {{
        MeshVertex{Vec3{0.0F, 0.0F, 0.0F}, Vec3{0.0F, 0.0F, 1.0F}, Vec2{0.0F, 0.0F}},
        MeshVertex{Vec3{1.0F, 0.0F, 0.0F}, Vec3{0.0F, 0.0F, 1.0F}, Vec2{1.0F, 0.0F}},
        MeshVertex{Vec3{0.0F, 1.0F, 0.0F}, Vec3{0.0F, 0.0F, 1.0F}, Vec2{0.0F, 1.0F}}
    }};
}

void check_triangle_unchanged(const storm::render::MeshData& mesh,
                              const storm::render::MeshVertex& first_vertex) {
    check(mesh.vertex_count() == 3U, "failed update preserves vertex count");
    check(mesh.index_count() == 3U, "failed update preserves index count");
    check(mesh.triangle_count() == 1U, "failed update preserves triangle count");
    check(!mesh.empty(), "failed update preserves non-empty mesh");
    if (mesh.vertex_count() == 3U) {
        check(mesh.vertices()[0U] == first_vertex, "failed update preserves prior vertex data");
    }
    if (mesh.index_count() == 3U) {
        check(mesh.indices()[0U] == 0U && mesh.indices()[1U] == 1U && mesh.indices()[2U] == 2U,
              "failed update preserves prior index data");
    }
}

} // namespace

int main() {
    using storm::core::ErrorCode;
    using storm::math::Vec2;
    using storm::math::Vec3;
    using storm::render::MeshData;

    MeshData mesh;
    check(mesh.empty(), "new mesh is empty");
    check(mesh.vertex_count() == 0U && mesh.index_count() == 0U,
          "new mesh has no vertices or indices");

    const auto source_vertices = make_triangle();
    const std::array<std::uint32_t, 3U> source_indices{0U, 1U, 2U};
    const auto success = mesh.set_geometry(source_vertices, source_indices);
    check(success.ok(), "valid triangle is accepted");
    check(mesh.vertex_count() == 3U && mesh.index_count() == 3U && mesh.triangle_count() == 1U,
          "valid triangle counts are reported");
    check(mesh.vertices()[0U].position == Vec3{0.0F, 0.0F, 0.0F},
          "mesh owns a copy of vertex data");
    check(mesh.indices()[2U] == 2U, "mesh owns index data");
    const auto original_first_vertex = mesh.vertices()[0U];

    auto non_finite_vertices = source_vertices;
    non_finite_vertices[1U].normal[2U] = std::numeric_limits<float>::infinity();
    auto status = mesh.set_geometry(non_finite_vertices, source_indices);
    check(!status.ok() && status.code() == ErrorCode::InvalidArgument,
          "non-finite normal is rejected");
    check_triangle_unchanged(mesh, original_first_vertex);

    non_finite_vertices = source_vertices;
    non_finite_vertices[0U].position[0U] = std::numeric_limits<float>::quiet_NaN();
    status = mesh.set_geometry(non_finite_vertices, source_indices);
    check(!status.ok() && status.code() == ErrorCode::InvalidArgument,
          "non-finite position is rejected");
    check_triangle_unchanged(mesh, original_first_vertex);

    non_finite_vertices = source_vertices;
    non_finite_vertices[2U].texcoord[1U] = std::numeric_limits<float>::infinity();
    status = mesh.set_geometry(non_finite_vertices, source_indices);
    check(!status.ok() && status.code() == ErrorCode::InvalidArgument,
          "non-finite UV coordinate is rejected");
    check_triangle_unchanged(mesh, original_first_vertex);

    const std::array<std::uint32_t, 3U> out_of_range_indices{0U, 1U, 3U};
    status = mesh.set_geometry(source_vertices, out_of_range_indices);
    check(!status.ok() && status.code() == ErrorCode::InvalidArgument,
          "out-of-range index is rejected");
    check_triangle_unchanged(mesh, original_first_vertex);

    const std::array<std::uint32_t, 2U> incomplete_indices{0U, 1U};
    status = mesh.set_geometry(source_vertices, incomplete_indices);
    check(!status.ok() && status.code() == ErrorCode::InvalidArgument,
          "index list that does not form complete triangles is rejected");
    check_triangle_unchanged(mesh, original_first_vertex);

    status = mesh.set_geometry({}, source_indices);
    check(!status.ok() && status.code() == ErrorCode::InvalidArgument,
          "empty vertex input is rejected");
    check_triangle_unchanged(mesh, original_first_vertex);

    mesh.reset();
    check(mesh.empty(), "reset clears mesh geometry");
    check(mesh.vertex_count() == 0U && mesh.index_count() == 0U && mesh.triangle_count() == 0U,
          "reset clears all geometry counts");

    // A finite zero normal is permitted for data pipelines that generate normals later.
    auto zero_normal_vertices = make_triangle();
    zero_normal_vertices[0U].normal = Vec3{0.0F, 0.0F, 0.0F};
    const auto zero_normal_status = mesh.set_geometry(zero_normal_vertices, source_indices);
    check(zero_normal_status.ok(), "finite zero normal is retained without implicit normalization");
    check(mesh.vertices()[0U].texcoord == Vec2{0.0F, 0.0F}, "UV data is retained unchanged");

    if (failures != 0) {
        std::cerr << failures << " mesh data test(s) failed\n";
        return 1;
    }
    std::cout << "All mesh data tests passed\n";
    return 0;
}
