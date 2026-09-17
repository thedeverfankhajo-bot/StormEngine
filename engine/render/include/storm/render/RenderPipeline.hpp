#pragma once

#include "Material.hpp"
#include "RenderTypes.hpp"

namespace storm::render {

class RenderPipeline final {
public:
    RenderPipeline() = default;

    RenderPipeline(ShaderHandle vertexShader, ShaderHandle fragmentShader,
                   MaterialHandle material,
                   PrimitiveTopology topology = PrimitiveTopology::Triangles) noexcept
        : vertexShader_(vertexShader), fragmentShader_(fragmentShader),
          material_(material), topology_(topology) {}

    [[nodiscard]] ShaderHandle vertexShader() const noexcept { return vertexShader_; }
    [[nodiscard]] ShaderHandle fragmentShader() const noexcept { return fragmentShader_; }
    [[nodiscard]] ShaderHandle shader() const noexcept { return vertexShader_; }
    [[nodiscard]] MaterialHandle material() const noexcept { return material_; }
    [[nodiscard]] PrimitiveTopology topology() const noexcept { return topology_; }
    [[nodiscard]] bool valid() const noexcept {
        return vertexShader_.valid() && fragmentShader_.valid() && material_.valid();
    }

    void setVertexShader(ShaderHandle shader) noexcept { vertexShader_ = shader; }
    void setFragmentShader(ShaderHandle shader) noexcept { fragmentShader_ = shader; }
    void setShader(ShaderHandle shader) noexcept { vertexShader_ = shader; }
    void setMaterial(MaterialHandle material) noexcept { material_ = material; }
    void setTopology(PrimitiveTopology topology) noexcept { topology_ = topology; }

    [[nodiscard]] DrawCommand apply(const DrawCommand& command) const noexcept {
        DrawCommand result = command;
        result.shader = vertexShader_;
        result.fragmentShader = fragmentShader_;
        result.material = material_;
        result.topology = topology_;
        return result;
    }

    [[nodiscard]] DrawCommand apply(const DrawCommand& command,
                                    const Material& material) const noexcept {
        DrawCommand result = apply(command);
        result.materialData = &material;
        return result;
    }

private:
    ShaderHandle vertexShader_{};
    ShaderHandle fragmentShader_{};
    MaterialHandle material_{};
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
};

} // namespace storm::render
