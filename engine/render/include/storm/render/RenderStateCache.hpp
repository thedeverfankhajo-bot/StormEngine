#pragma once
#include "RenderTypes.hpp"
#include <cstdint>
namespace storm::render {
struct RenderState final {
    ShaderHandle shader{};
    ShaderHandle fragmentShader{};
    PrimitiveTopology topology{PrimitiveTopology::Triangles};
    bool blendEnabled{false};
    bool depthTestEnabled{true};
    bool depthWriteEnabled{true};
    bool cullEnabled{true};
    bool scissorEnabled{false};
    std::uint32_t viewportWidth{0};
    std::uint32_t viewportHeight{0};
    std::int32_t scissorX{0};
    std::int32_t scissorY{0};
    std::uint32_t scissorWidth{0};
    std::uint32_t scissorHeight{0};
    friend constexpr bool operator==(const RenderState&, const RenderState&) noexcept = default;
};
class RenderStateCache final {
public:
    void invalidate() noexcept { valid_ = false; }
    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] const RenderState& state() const noexcept { return state_; }
    [[nodiscard]] bool needsApply(const RenderState& desired) const noexcept { return !valid_ || state_ != desired; }
    void markApplied(const RenderState& applied) noexcept { state_ = applied; valid_ = true; }
private:
    RenderState state_{};
    bool valid_{false};
};
} // namespace storm::render
