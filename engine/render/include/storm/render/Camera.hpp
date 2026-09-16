#pragma once

#include "storm/math/Mat4.hpp"
#include "storm/math/Quaternion.hpp"
#include "storm/math/Vec3.hpp"

namespace storm::render {

class Camera {
public:
    enum class ProjectionType {
        Perspective,
        Orthographic
    };

    Camera() = default;

    void setPosition(const math::Vec3& position) { position_ = position; }
    void setRotation(const math::Quaternion& rotation) { rotation_ = rotation.normalized(); }

    const math::Vec3& position() const noexcept { return position_; }
    const math::Quaternion& rotation() const noexcept { return rotation_; }

    void setPerspective(float verticalFovRadians, float aspect, float nearPlane, float farPlane) {
        projectionType_ = ProjectionType::Perspective;
        verticalFovRadians_ = verticalFovRadians;
        aspect_ = aspect;
        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    void setOrthographic(float left, float right, float bottom, float top,
                         float nearPlane, float farPlane) {
        projectionType_ = ProjectionType::Orthographic;
        left_ = left;
        right_ = right;
        bottom_ = bottom;
        top_ = top;
        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    ProjectionType projectionType() const noexcept { return projectionType_; }

    math::Mat4 viewMatrix() const {
        return rotation_.inverse().toMat4() * math::Mat4::translation(-position_);
    }

    math::Mat4 projectionMatrix() const {
        if (projectionType_ == ProjectionType::Perspective) {
            return math::Mat4::perspective(verticalFovRadians_, aspect_, nearPlane_, farPlane_);
        }
        return math::Mat4::orthographic(left_, right_, bottom_, top_, nearPlane_, farPlane_);
    }

    math::Mat4 viewProjectionMatrix() const {
        return projectionMatrix() * viewMatrix();
    }

    math::Vec3 forward() const { return rotation_.rotate(math::Vec3::forward()); }
    math::Vec3 right() const { return rotation_.rotate(math::Vec3::right()); }
    math::Vec3 up() const { return rotation_.rotate(math::Vec3::up()); }

private:
    math::Vec3 position_{math::Vec3::zero()};
    math::Quaternion rotation_{math::Quaternion::identity()};

    ProjectionType projectionType_{ProjectionType::Perspective};
    float verticalFovRadians_{1.0471975512f};
    float aspect_{16.0f / 9.0f};
    float nearPlane_{0.1f};
    float farPlane_{1000.0f};

    float left_{-1.0f};
    float right_{1.0f};
    float bottom_{-1.0f};
    float top_{1.0f};
};

} // namespace storm::render
