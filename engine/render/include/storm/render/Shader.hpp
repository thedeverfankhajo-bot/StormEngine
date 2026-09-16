#pragma once

#include "RenderTypes.hpp"
#include <string>
#include <utility>

namespace storm::render {

class Shader final {
public:
    Shader() = default;
    explicit Shader(const ShaderDesc& desc, std::string source = {})
        : desc_(desc), source_(std::move(source)) {}

    [[nodiscard]] const ShaderDesc& description() const noexcept { return desc_; }
    [[nodiscard]] const std::string& source() const noexcept { return source_; }
    [[nodiscard]] bool valid() const noexcept { return !source_.empty(); }

private:
    ShaderDesc desc_{};
    std::string source_;
};

} // namespace storm::render
