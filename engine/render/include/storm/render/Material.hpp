#pragma once

#include "RenderTypes.hpp"

namespace storm::render {

class Material final {
public:
    Material() = default;
    explicit Material(MaterialHandle handle, ShaderHandle shader = {}) noexcept
        : handle_(handle), shader_(shader) {}

    [[nodiscard]] MaterialHandle handle() const noexcept { return handle_; }
    [[nodiscard]] ShaderHandle shader() const noexcept { return shader_; }
    [[nodiscard]] bool valid() const noexcept { return handle_.valid(); }

    void setHandle(MaterialHandle handle) noexcept { handle_ = handle; }
    void setShader(ShaderHandle shader) noexcept { shader_ = shader; }

private:
    MaterialHandle handle_{};
    ShaderHandle shader_{};
};

} // namespace storm::render
