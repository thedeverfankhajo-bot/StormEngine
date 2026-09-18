#pragma once

#include "storm/math/Mat4.hpp"
#include "storm/math/Quaternion.hpp"
#include "storm/math/Vec3.hpp"
#include <cmath>

namespace storm::render {

class Camera3D final {
public:
    void setPosition(math::Vec3 position) noexcept {
        position_ = finite(position) ? position : math::Vec3{};
    }

    void setRotation(math::Quaternion rotation) noexcept {
        rotation_ = finite(rotation) ? rotation.normalized() : math::Quaternion::identity();
    }

    void setViewport(float width, float height) noexcept {
        viewportWidth_ = width > 0.0f && std::isfinite(width) ? width : 1.0f;
        viewportHeight_ = height > 0.0f && std::isfinite(height) ? height : 1.0f;
    }

    void setPerspective(float verticalFovRadians, float nearPlane, float farPlane) noexcept {
        fovRadians_ = verticalFovRadians > 0.0f && verticalFovRadians < 3.1415926535f &&
                      std::isfinite(verticalFovRadians) ? verticalFovRadians : 1.0471975512f;
        nearPlane_ = nearPlane > 0.0f && std::isfinite(nearPlane) ? nearPlane : 0.01f;
        farPlane_ = farPlane > nearPlane_ && std::isfinite(farPlane) ? farPlane : 1000.0f;
        if (farPlane_ <= nearPlane_) farPlane_ = nearPlane_ + 1.0f;
    }

    [[nodiscard]] math::Vec3 position() const noexcept { return position_; }
    [[nodiscard]] math::Quaternion rotation() const noexcept { return rotation_; }
    [[nodiscard]] float viewportWidth() const noexcept { return viewportWidth_; }
    [[nodiscard]] float viewportHeight() const noexcept { return viewportHeight_; }
    [[nodiscard]] float fieldOfViewRadians() const noexcept { return fovRadians_; }
    [[nodiscard]] float nearPlane() const noexcept { return nearPlane_; }
    [[nodiscard]] float farPlane() const noexcept { return farPlane_; }

    [[nodiscard]] math::Mat4 viewMatrix() const {
        return rotation_.inverse().toMat4() * math::Mat4::translation(-position_);
    }

    [[nodiscard]] math::Mat4 projectionMatrix() const {
        return math::Mat4::perspective(fovRadians_, viewportWidth_ / viewportHeight_, nearPlane_, farPlane_);
    }

    [[nodiscard]] math::Mat4 viewProjectionMatrix() const {
        return projectionMatrix() * viewMatrix();
    }

    [[nodiscard]] math::Vec3 forward() const { return rotation_.rotate(math::Vec3::forward()); }
    [[nodiscard]] math::Vec3 right() const { return rotation_.rotate(math::Vec3::right()); }
    [[nodiscard]] math::Vec3 up() const { return rotation_.rotate(math::Vec3::up()); }

private:
    static bool finite(math::Vec3 value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    static bool finite(math::Quaternion value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) &&
               std::isfinite(value.z) && std::isfinite(value.w);
    }

    math::Vec3 position_{};
    math::Quaternion rotation_{};
    float viewportWidth_{1280.0f};
    float viewportHeight_{720.0f};
    float fovRadians_{1.0471975512f};
    float nearPlane_{0.01f};
    float farPlane_{1000.0f};
};

} // namespace storm::render
