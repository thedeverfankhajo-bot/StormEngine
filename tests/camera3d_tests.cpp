#include <cassert>
#include <cmath>

#include "storm/render/Camera3D.hpp"

static bool near(float a, float b, float eps = 0.0001f) {
    return std::fabs(a - b) <= eps;
}

int main() {
    storm::render::Camera3D camera;
    camera.setViewport(1920.0f, 1080.0f);
    camera.setPosition({1.0f, 2.0f, 3.0f});

    const auto view = camera.viewMatrix();
    assert(near(view.m[0][3], -1.0f));
    assert(near(view.m[1][3], -2.0f));
    assert(near(view.m[2][3], -3.0f));

    const auto projection = camera.projectionMatrix();
    assert(projection.m[0][0] > 0.0f);
    assert(projection.m[1][1] > 0.0f);
    assert(near(projection.m[3][2], -1.0f));

    const auto vp = camera.viewProjectionMatrix();
    const auto clip = vp * storm::math::Vec4{0.0f, 0.0f, -5.0f, 1.0f};
    assert(std::isfinite(clip.x) && std::isfinite(clip.y) &&
           std::isfinite(clip.z) && std::isfinite(clip.w));

    camera.setRotation(storm::math::Quaternion::fromAxisAngle(storm::math::Vec3::up(), 0.5f));
    const auto forward = camera.forward();
    assert(std::isfinite(forward.x) && std::isfinite(forward.y) && std::isfinite(forward.z));

    camera.setViewport(-1.0f, 0.0f);
    assert(camera.viewportWidth() == 1.0f);
    assert(camera.viewportHeight() == 1.0f);
    camera.setPerspective(0.0f, -1.0f, 0.0f);
    assert(camera.fieldOfViewRadians() > 0.0f);
    assert(camera.nearPlane() > 0.0f);
    assert(camera.farPlane() > camera.nearPlane());
    return 0;
}
