#pragma once

#include "storm/math/Mat4.hpp"
#include <cmath>

namespace storm::render {

class Camera1D final {
public:
    void setPosition(float position) noexcept { position_ = std::isfinite(position) ? position : 0.0f; }
    void setViewport(float width) noexcept { viewportWidth_ = width > 0.0f && std::isfinite(width) ? width : 1.0f; }
    void setZoom(float zoom) noexcept { zoom_ = zoom > 0.0f && std::isfinite(zoom) ? zoom : 1.0f; }

    [[nodiscard]] float position() const noexcept { return position_; }
    [[nodiscard]] float viewportWidth() const noexcept { return viewportWidth_; }
    [[nodiscard]] float zoom() const noexcept { return zoom_; }

    [[nodiscard]] math::Mat4 viewMatrix() const {
        return math::Mat4::translation({-position_, 0.0f, 0.0f});
    }

    [[nodiscard]] math::Mat4 projectionMatrix() const {
        const float half = viewportWidth_ * 0.5f / zoom_;
        return math::Mat4::orthographic(-half, half, -0.5f, 0.5f, -1.0f, 1.0f);
    }

    [[nodiscard]] math::Mat4 viewProjectionMatrix() const { return projectionMatrix() * viewMatrix(); }

private:
    float position_{0.0f};
    float viewportWidth_{1280.0f};
    float zoom_{1.0f};
};

} // namespace storm::render
