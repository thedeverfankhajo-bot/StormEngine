#include <cassert>
#include <cmath>
#include "storm/render/Camera1D.hpp"

int main() {
    storm::render::Camera1D camera;
    assert(camera.position() == 0.0f);
    camera.setPosition(10.0f);
    camera.setViewport(320.0f);
    camera.setZoom(2.0f);
    assert(camera.position() == 10.0f);
    assert(camera.viewportWidth() == 320.0f);
    assert(camera.zoom() == 2.0f);
    const auto vp = camera.viewProjectionMatrix();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            assert(std::isfinite(vp.m[r][c]));
    camera.setZoom(NAN);
    camera.setViewport(-1.0f);
    camera.setPosition(INFINITY);
    assert(camera.zoom() == 1.0f && camera.viewportWidth() == 1.0f && camera.position() == 0.0f);
    return 0;
}
