#include <cassert>
#include <cmath>

#include "storm/render/Camera.hpp"

namespace {

constexpr float kTolerance = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) <= kTolerance;
}

void testDefaultCamera() {
    storm::render::Camera camera;
    assert(near(camera.position().x, 0.0f));
    assert(near(camera.position().y, 0.0f));
    assert(near(camera.position().z, 0.0f));

    const auto forward = camera.forward();
    assert(near(forward.x, 0.0f));
    assert(near(forward.y, 0.0f));
    assert(near(forward.z, -1.0f));
}

void testViewMatrix() {
    storm::render::Camera camera;
    camera.setPosition({0.0f, 0.0f, 5.0f});

    const auto view = camera.viewMatrix();
    const auto cameraSpace = view * storm::math::Vec4{0.0f, 0.0f, 5.0f, 1.0f};
    assert(near(cameraSpace.x, 0.0f));
    assert(near(cameraSpace.y, 0.0f));
    assert(near(cameraSpace.z, 0.0f));
    assert(near(cameraSpace.w, 1.0f));
}

void testViewProjection() {
    storm::render::Camera camera;
    camera.setPerspective(storm::math::radians(90.0f), 1.0f, 0.1f, 100.0f);
    camera.setPosition({0.0f, 0.0f, 5.0f});

    const auto clip = camera.viewProjectionMatrix() * storm::math::Vec4{0.0f, 0.0f, 4.0f, 1.0f};
    assert(clip.w > 0.0f);
    assert(clip.z / clip.w > -1.0f);
    assert(clip.z / clip.w < 1.0f);
}

} // namespace

int main() {
    testDefaultCamera();
    testViewMatrix();
    testViewProjection();
    return 0;
}
