#pragma once

#include "storm/math/Mat4.hpp"
#include "storm/math/Vec2.hpp"
#include <cmath>

namespace storm::render {

class Camera2D final {
public:
    Camera2D() = default;

    void setPosition(const math::Vec2& position) noexcept { position_ = position; }
    void setRotation(float radians) noexcept { rotationRadians_ = std::isfinite(radians) ? radians : 0.0f; }
    void setViewport(float width, float height) noexcept {
        viewportWidth_ = width > 0.0f && std::isfinite(width) ? width : 1.0f;
        viewportHeight_ = height > 0.0f && std::isfinite(height) ? height : 1.0f;
    }
    void setZoom(float zoom) noexcept { zoom_ = zoom > 0.0f && std::isfinite(zoom) ? zoom : 1.0f; }
    void setDepthRange(float nearPlane, float farPlane) noexcept {
        if (std::isfinite(nearPlane) && std::isfinite(farPlane) && farPlane != nearPlane) {
            nearPlane_ = nearPlane;
            farPlane_ = farPlane;
        }
    }

    [[nodiscard]] const math::Vec2& position() const noexcept { return position_; }
    [[nodiscard]] float rotation() const noexcept { return rotationRadians_; }
    [[nodiscard]] float viewportWidth() const noexcept { return viewportWidth_; }
    [[nodiscard]] float viewportHeight() const noexcept { return viewportHeight_; }
    [[nodiscard]] float zoom() const noexcept { return zoom_; }

    [[nodiscard]] math::Mat4 viewMatrix() const {
        return math::Mat4::rotationZ(-rotationRadians_) *
               math::Mat4::translation({-position_.x, -position_.y, 0.0f});
    }

    [[nodiscard]] math::Mat4 projectionMatrix() const {
        const float halfWidth = viewportWidth_ * 0.5f / zoom_;
        const float halfHeight = viewportHeight_ * 0.5f / zoom_;
        return math::Mat4::orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight,
                                        nearPlane_, farPlane_);
    }

    [[nodiscard]] math::Mat4 viewProjectionMatrix() const { return projectionMatrix() * viewMatrix(); }

private:
    math::Vec2 position_{};
    float rotationRadians_{0.0f};
    float viewportWidth_{1280.0f};
    float viewportHeight_{720.0f};
    float zoom_{1.0f};
    float nearPlane_{-1.0f};
    float farPlane_{1.0f};
};

} // namespace storm::render
