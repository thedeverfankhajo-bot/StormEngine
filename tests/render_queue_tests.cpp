#include <cassert>
#include <string>

#include "storm/render/RenderQueue.hpp"

namespace {
class RecordingDevice final : public storm::render::RenderDevice {
public:
    storm::render::BufferHandle createBuffer(const storm::render::BufferDesc&) override { return {}; }
    void destroyBuffer(storm::render::BufferHandle) override {}
    bool updateBuffer(storm::render::BufferHandle, const void*, std::size_t, std::size_t) override { return false; }
    storm::render::TextureHandle createTexture(const storm::render::TextureDesc&) override { return {}; }
    void destroyTexture(storm::render::TextureHandle) override {}
    storm::render::ShaderHandle createShader(const storm::render::ShaderDesc&, const std::string&) override { return {}; }
    void destroyShader(storm::render::ShaderHandle) override {}
    void beginFrame() override { began = true; ended = false; }
    bool submit(const storm::render::DrawCommand& command) override {
        ++submits;
        return command.vertexBuffer.valid() && command.vertexCount > 0 && command.vertexLayout.valid();
    }
    void endFrame() override { ended = true; }
    std::size_t liveBufferCount() const noexcept override { return 0; }
    std::size_t liveTextureCount() const noexcept override { return 0; }
    std::size_t liveShaderCount() const noexcept override { return 0; }
    std::size_t submittedDrawCount() const noexcept override { return submits; }

    bool began{false};
    bool ended{false};
    std::size_t submits{0};
};

storm::render::VertexLayout layout() {
    storm::render::VertexLayout result{};
    result.attributes[0] = {0, storm::render::VertexFormat::Float32x3, 0};
    result.stride = 12;
    result.attributeCount = 1;
    return result;
}
}

int main() {
    using namespace storm::render;

    RenderQueue queue;
    assert(queue.empty());
    assert(queue.size() == 0);

    DrawCommand first{};
    first.vertexBuffer = BufferHandle(1);
    first.vertexCount = 3;
    first.vertexLayout = layout();
    assert(queue.submit(first));

    DrawCommand second{};
    second.topology = PrimitiveTopology::Lines;
    second.vertexBuffer = BufferHandle(2);
    second.vertexCount = 6;
    second.vertexLayout = layout();
    assert(queue.submit(second));

    assert(!queue.empty());
    assert(queue.size() == 2);
    assert(queue.at(0).vertexBuffer == BufferHandle(1));
    assert(queue.at(0).vertexCount == 3);
    assert(queue.at(1).topology == PrimitiveTopology::Lines);
    assert(queue.at(1).vertexCount == 6);
    assert(queue.commands().size() == 2);

    DrawCommand invalid{};
    invalid.vertexBuffer = BufferHandle(99);
    invalid.vertexCount = 3;
    assert(!queue.submit(invalid));
    assert(queue.size() == 2);

    queue.beginFrame();
    assert(queue.empty());
    assert(queue.size() == 0);

    DrawCommand indexed{};
    indexed.vertexBuffer = BufferHandle(4);
    indexed.indexBuffer = BufferHandle(5);
    indexed.indexCount = 12;
    indexed.vertexCount = 3;
    indexed.vertexLayout = layout();
    assert(queue.submit(indexed));
    assert(queue.size() == 1);
    assert(queue.at(0).indexed());
    DrawCommand badBase = first;
    badBase.baseVertex = 1;
    assert(!queue.submit(badBase));

    DrawCommand badViewport = first;
    badViewport.viewportWidth = 1280;
    assert(!queue.submit(badViewport));

    RecordingDevice device;
    assert(queue.execute(device));
    assert(device.began);
    assert(device.ended);
    assert(device.submits == 1);

    return 0;
}
