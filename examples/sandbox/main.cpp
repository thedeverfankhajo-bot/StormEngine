#include <iostream>

#include "storm/core/Engine.hpp"
#include "storm/render/NullRenderDevice.hpp"

int main() {
    const storm::Engine engine;
    storm::render::NullRenderDevice device;

    const auto vertexBuffer = device.createBuffer({1024, storm::render::BufferUsage::Static});
    const auto vertexShader = device.createShader(
        {storm::render::ShaderStage::Vertex}, "void main() {}");
    const auto fragmentShader = device.createShader(
        {storm::render::ShaderStage::Fragment}, "void main() {}");

    if (!vertexBuffer.valid() || !vertexShader.valid() || !fragmentShader.valid()) {
        std::cerr << "failed to create sandbox resources\n";
        return 1;
    }

    storm::render::VertexLayout layout{};
    layout.attributeCount = 1;
    layout.stride = 12;
    layout.attributes[0] = {
        0, storm::render::VertexFormat::Float32x3, 0
    };

    storm::render::DrawCommand draw{};
    draw.vertexBuffer = vertexBuffer;
    draw.vertexCount = 3;
    draw.shader = vertexShader;
    draw.fragmentShader = fragmentShader;
    draw.vertexLayout = layout;

    device.beginFrame();
    const bool submitted = device.submit(draw);
    device.endFrame();

    if (!submitted) {
        std::cerr << "failed to submit draw command\n";
        return 2;
    }

    device.destroyShader(fragmentShader);
    device.destroyShader(vertexShader);
    device.destroyBuffer(vertexBuffer);

    std::cout << engine.name() << ' ' << engine.version()
              << " | headless frame: draws=" << device.submittedDrawCount()
              << " buffers=" << device.liveBufferCount() << '\n';

    return 0;
}
