#include <cassert>
#include <cmath>

#include "storm/render/Camera2D.hpp"

int main() {
    using namespace storm::render;

    Camera2D camera;
    assert(camera.viewportWidth() == 1280.0f);
    assert(camera.viewportHeight() == 720.0f);
    assert(camera.zoom() == 1.0f);

    camera.setViewport(320.0f, 180.0f);
    camera.setPosition({10.0f, 20.0f});
    camera.setZoom(2.0f);
    camera.setRotation(0.25f);

    const auto projection = camera.projectionMatrix();
    assert(std::isfinite(projection.m[0][0]));
    assert(std::isfinite(projection.m[1][1]));
    assert(std::fabs(projection.m[0][0] - 0.0125f) < 1e-6f);
    assert(std::fabs(projection.m[1][1] - 0.0222222f) < 1e-5f);

    camera.setZoom(0.0f);
    assert(camera.zoom() == 1.0f);
    camera.setViewport(-10.0f, 0.0f);
    assert(camera.viewportWidth() == 1.0f);
    assert(camera.viewportHeight() == 1.0f);
    camera.setRotation(NAN);
    assert(camera.rotation() == 0.0f);

    const auto vp = camera.viewProjectionMatrix();
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            assert(std::isfinite(vp.m[row][col]));

    return 0;
}
