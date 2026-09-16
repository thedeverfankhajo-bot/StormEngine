#include <iostream>

#include "storm/core/Engine.hpp"
#include "storm/render/NullRenderDevice.hpp"

int main() {
    const storm::Engine engine;
    storm::render::NullRenderDevice device;

    const auto vertexBuffer = device.createBuffer({1024, storm::render::BufferUsage::Static});
    if (!vertexBuffer.valid()) {
        std::cerr << "failed to create vertex buffer\n";
        return 1;
    }

    device.beginFrame();

    storm::render::DrawCommand draw{};
    draw.vertexBuffer = vertexBuffer;
    draw.vertexCount = 3;

    if (!device.submit(draw)) {
        std::cerr << "failed to submit draw command\n";
        return 2;
    }

    device.endFrame();

    std::cout << engine.name() << ' ' << engine.version()
              << " | headless frame: draws=" << device.submittedDrawCount()
              << " buffers=" << device.liveBufferCount() << '\n';

    return 0;
}
