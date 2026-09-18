#pragma once

#include "Material.hpp"
#include "Mesh.hpp"

namespace storm::render {

struct Renderable final {
    Mesh mesh{};
    Material material{};
    bool visible{true};

    Renderable() = default;
    Renderable(Mesh meshValue, Material materialValue) noexcept
        : mesh(std::move(meshValue)), material(std::move(materialValue)) {}

    [[nodiscard]] bool valid() const noexcept {
        return visible && mesh.valid() && material.valid() &&
               material.shader().valid();
    }
};

} // namespace storm::render
