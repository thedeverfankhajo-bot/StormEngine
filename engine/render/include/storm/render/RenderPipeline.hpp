#pragma once

#include "RenderTypes.hpp"

namespace storm::render {

class RenderPipeline final {
public:
    RenderPipeline() = default;
    RenderPipeline(ShaderHandle shader, MaterialHandle material,
                   PrimitiveTopology topology = PrimitiveTopology::Triangles) noexcept
        : shader_(shader), material_(material), topology_(topology) {}

    [[nodiscard]] ShaderHandle shader() const noexcept { return shader_; }
    [[nodiscard]] MaterialHandle material() const noexcept { return material_; }
    [[nodiscard]] PrimitiveTopology topology() const noexcept { return topology_; }
    [[nodiscard]] bool valid() const noexcept { return shader_.valid() && material_.valid(); }

    void setShader(ShaderHandle shader) noexcept { shader_ = shader; }
    void setMaterial(MaterialHandle material) noexcept { material_ = material; }
    void setTopology(PrimitiveTopology topology) noexcept { topology_ = topology; }

    [[nodiscard]] DrawCommand apply(const DrawCommand& command) const noexcept {
        DrawCommand result = command;
        result.shader = shader_;
        result.material = material_;
        result.topology = topology_;
        return result;
    }

private:
    ShaderHandle shader_{};
    MaterialHandle material_{};
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
};

} // namespace storm::render
