#include <cassert>

#include "storm/render/NullRenderDevice.hpp"

int main() {
    using namespace storm::render;

    NullRenderDevice device;
    assert(device.liveBufferCount() == 0);
    assert(device.liveTextureCount() == 0);
    assert(device.liveShaderCount() == 0);
    assert(device.submittedDrawCount() == 0);

    assert(!device.createBuffer(BufferDesc{}).valid());
    assert(!device.createTexture(TextureDesc{0, 64, 1}).valid());
    assert(!device.createTexture(TextureDesc{64, 64, 0}).valid());
    assert(!device.createShader(ShaderDesc{ShaderStage::Vertex}, "").valid());

    const auto vertexBuffer = device.createBuffer(BufferDesc{1024, BufferUsage::Static});
    const auto indexBuffer = device.createBuffer(BufferDesc{512, BufferUsage::Static});
    const auto texture = device.createTexture(TextureDesc{64, 64, 1});
    const auto vertexShader = device.createShader(ShaderDesc{ShaderStage::Vertex}, "void main() {}");
    const auto fragmentShader = device.createShader(ShaderDesc{ShaderStage::Fragment}, "void main() {}");
    assert(vertexBuffer.valid());
    assert(indexBuffer.valid());
    assert(texture.valid());
    assert(vertexShader.valid());
    assert(fragmentShader.valid());
    assert(device.liveBufferCount() == 2);
    assert(device.liveTextureCount() == 1);
    assert(device.liveShaderCount() == 2);

    DrawCommand draw{};
    draw.vertexBuffer = vertexBuffer;
    draw.vertexCount = 3;
    draw.shader = vertexShader;
    draw.fragmentShader = fragmentShader;

    assert(!device.submit(draw));

    device.beginFrame();
    assert(device.submittedDrawCount() == 0);
    assert(device.submit(draw));
    assert(device.submittedDrawCount() == 1);

    DrawCommand indexed = draw;
    indexed.indexBuffer = indexBuffer;
    indexed.indexCount = 3;
    assert(device.submit(indexed));
    assert(device.submittedDrawCount() == 2);

    DrawCommand missingVertex = draw;
    missingVertex.vertexBuffer = BufferHandle{};
    assert(!device.submit(missingVertex));

    DrawCommand missingIndex = indexed;
    missingIndex.indexBuffer = BufferHandle{};
    assert(!device.submit(missingIndex));

    DrawCommand missingFragment = draw;
    missingFragment.fragmentShader = ShaderHandle{};
    assert(!device.submit(missingFragment));

    DrawCommand deadVertex = draw;
    device.destroyBuffer(vertexBuffer);
    assert(device.liveBufferCount() == 1);
    assert(!device.submit(deadVertex));

    DrawCommand deadShader = indexed;
    device.destroyShader(fragmentShader);
    assert(device.liveShaderCount() == 1);
    assert(!device.submit(deadShader));

    device.endFrame();
    assert(device.submittedDrawCount() == 2);
    assert(!device.submit(draw));

    device.destroyShader(vertexShader);
    device.destroyBuffer(indexBuffer);
    device.destroyTexture(texture);
    assert(device.liveShaderCount() == 0);
    assert(device.liveBufferCount() == 0);
    assert(device.liveTextureCount() == 0);

    device.destroyBuffer(indexBuffer);
    device.destroyTexture(texture);

    device.beginFrame();
    assert(device.submittedDrawCount() == 0);
    device.endFrame();

    return 0;
}
