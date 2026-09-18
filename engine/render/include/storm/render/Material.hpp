#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <utility>
#include "RenderTypes.hpp"

namespace storm::render {

using MaterialScalar = float;
using MaterialVec2 = std::array<float, 2>;
using MaterialVec3 = std::array<float, 3>;
using MaterialVec4 = std::array<float, 4>;
using MaterialMat4 = std::array<float, 16>;
using MaterialValue = std::variant<MaterialScalar, MaterialVec2, MaterialVec3, MaterialVec4,
                                   MaterialMat4, TextureHandle>;

class Material final {
public:
    using ParameterMap = std::unordered_map<std::string, MaterialValue>;

    Material() = default;
    explicit Material(MaterialHandle handle, ShaderHandle shader = {}) noexcept
        : handle_(handle), shader_(shader) {}

    [[nodiscard]] MaterialHandle handle() const noexcept { return handle_; }
    [[nodiscard]] ShaderHandle shader() const noexcept { return shader_; }
    [[nodiscard]] bool valid() const noexcept { return handle_.valid(); }

    void setHandle(MaterialHandle handle) noexcept { handle_ = handle; }
    void setShader(ShaderHandle shader) noexcept { shader_ = shader; }

    void setParameter(std::string name, MaterialValue value) {
        parameters_[std::move(name)] = std::move(value);
    }

    template <typename T>
    void setParameter(std::string name, T value) {
        setParameter(std::move(name), MaterialValue{std::move(value)});
    }

    [[nodiscard]] const MaterialValue* parameter(const std::string& name) const noexcept {
        const auto it = parameters_.find(name);
        return it == parameters_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] bool hasParameter(const std::string& name) const noexcept {
        return parameters_.find(name) != parameters_.end();
    }

    [[nodiscard]] std::size_t parameterCount() const noexcept { return parameters_.size(); }
    [[nodiscard]] const ParameterMap& parameters() const noexcept { return parameters_; }

    bool removeParameter(const std::string& name) noexcept {
        return parameters_.erase(name) != 0;
    }

    void clearParameters() noexcept { parameters_.clear(); }

private:
    MaterialHandle handle_{};
    ShaderHandle shader_{};
    ParameterMap parameters_;
};

} // namespace storm::render
