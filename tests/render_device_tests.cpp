#include <cassert>
#include <cstdint>

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
    assert(!device.createTexture(TextureDesc{3, 3, 3}).valid());
    assert(!device.createTexture(TextureDesc{64, 64, 8}).valid());
    assert(!device.createShader(ShaderDesc{ShaderStage::Vertex}, "").valid());

    const auto vertexBuffer = device.createBuffer(BufferDesc{1024, BufferUsage::Static});
    const auto indexBuffer = device.createBuffer(BufferDesc{512, BufferUsage::Static});
    const auto texture = device.createTexture(TextureDesc{64, 64, 2});
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

    std::uint8_t bytes[16]{};
    assert(device.updateBuffer(vertexBuffer, bytes, sizeof(bytes), 0));
    assert(device.updateBuffer(vertexBuffer, bytes, sizeof(bytes), 1008));
    assert(!device.updateBuffer(vertexBuffer, bytes, sizeof(bytes), 1009));
    assert(!device.updateBuffer(vertexBuffer, bytes, 0, 0));
    assert(!device.updateBuffer(vertexBuffer, nullptr, sizeof(bytes), 0));
    assert(!device.updateBuffer(BufferHandle{}, bytes, sizeof(bytes), 0));
    assert(!device.updateBuffer(vertexBuffer, bytes, sizeof(bytes), 1024));

    std::uint8_t rgba64[64 * 64 * 4]{};
    std::uint8_t rgba32[32 * 32 * 4]{};
    assert(device.updateTexture(texture, rgba64, sizeof(rgba64), 0));
    assert(device.updateTexture(texture, rgba32, sizeof(rgba32), 1));
    assert(!device.updateTexture(texture, rgba32, sizeof(rgba32) - 1, 1));
    assert(!device.updateTexture(texture, rgba64, sizeof(rgba64), 2));
    assert(!device.updateTexture(texture, nullptr, sizeof(rgba64), 0));
    assert(!device.updateTexture(TextureHandle{}, rgba64, sizeof(rgba64), 0));
    device.destroyTexture(texture);
    assert(!device.updateTexture(texture, rgba64, sizeof(rgba64), 0));

    VertexLayout layout{};
    layout.attributeCount = 1;
    layout.stride = sizeof(float) * 3;
    layout.attributes[0] = {0, VertexFormat::Float32x3, 0};
    assert(layout.valid());

    DrawCommand draw{};
    draw.vertexBuffer = vertexBuffer;
    draw.vertexCount = 3;
    draw.shader = vertexShader;
    draw.fragmentShader = fragmentShader;
    draw.vertexLayout = layout;

    assert(!device.submit(draw));
    DrawCommand wrongStage = draw;
    wrongStage.shader = fragmentShader;
    wrongStage.fragmentShader = vertexShader;
    device.beginFrame();
    assert(!device.submit(wrongStage));
    device.endFrame();

    device.beginFrame();
    assert(device.submittedDrawCount() == 0);
    assert(device.submit(draw));
    assert(device.submittedDrawCount() == 1);

    DrawCommand indexed = draw;
    indexed.indexBuffer = indexBuffer;
    indexed.indexCount = 3;
    assert(device.submit(indexed));

    DrawCommand staleIndex = indexed;
    device.destroyBuffer(indexBuffer);
    assert(!device.submit(staleIndex));
    const auto replacementIndexBuffer = device.createBuffer(BufferDesc{512, BufferUsage::Static});
    assert(replacementIndexBuffer.valid());
    staleIndex.indexBuffer = replacementIndexBuffer;
    assert(device.submit(staleIndex));
    assert(device.submittedDrawCount() == 3);

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
    assert(!device.updateBuffer(vertexBuffer, bytes, sizeof(bytes), 0));

    DrawCommand deadShader = indexed;
    device.destroyShader(fragmentShader);
    assert(device.liveShaderCount() == 1);
    assert(!device.submit(deadShader));

    device.endFrame();
    assert(device.submittedDrawCount() == 3);
    assert(!device.submit(draw));

    device.destroyShader(vertexShader);
    device.destroyBuffer(replacementIndexBuffer);
    device.destroyTexture(texture);
    assert(device.liveShaderCount() == 0);
    assert(device.liveBufferCount() == 0);
    assert(device.liveTextureCount() == 0);

    // Stale renderer handles must never destroy or mutate a replacement resource.
    const staleBuffer = replacementIndexBuffer;
    device.destroyBuffer(staleBuffer);
    const replacementBuffer = device.createBuffer(BufferDesc{512, BufferUsage::Static});
    assert(replacementBuffer.valid());
    assert(replacementBuffer.id() == staleBuffer.id());
    assert(replacementBuffer.generation() != staleBuffer.generation());
    std::uint8_t replacementBytes[8]{};
    assert(!device.updateBuffer(staleBuffer, replacementBytes, sizeof(replacementBytes), 0));
    device.destroyBuffer(staleBuffer);
    assert(device.updateBuffer(replacementBuffer, replacementBytes, sizeof(replacementBytes), 0));
    device.destroyBuffer(replacementBuffer);

    const auto staleTexture = device.createTexture(TextureDesc{2, 2, 1});
    assert(staleTexture.valid());
    device.destroyTexture(staleTexture);
    const auto replacementTexture = device.createTexture(TextureDesc{2, 2, 1});
    assert(replacementTexture.valid());
    assert(replacementTexture.id() == staleTexture.id());
    assert(replacementTexture.generation() != staleTexture.generation());
    std::uint8_t replacementPixels[16]{};
    assert(!device.updateTexture(staleTexture, replacementPixels, sizeof(replacementPixels), 0));
    device.destroyTexture(staleTexture);
    assert(device.updateTexture(replacementTexture, replacementPixels, sizeof(replacementPixels), 0));
    device.destroyTexture(replacementTexture);

    const auto staleShader = device.createShader(ShaderDesc{ShaderStage::Vertex}, "void main() {}");
    assert(staleShader.valid());
    device.destroyShader(staleShader);
    const auto replacementShader = device.createShader(ShaderDesc{ShaderStage::Vertex}, "void main() {}");
    assert(replacementShader.valid());
    assert(replacementShader.id() == staleShader.id());
    assert(replacementShader.generation() != staleShader.generation());
    device.destroyShader(staleShader);
    assert(device.liveShaderCount() == 1);
    device.destroyShader(replacementShader);
    assert(device.liveShaderCount() == 0);

    device.destroyBuffer(indexBuffer);
    device.destroyTexture(texture);

    device.beginFrame();
    assert(device.submittedDrawCount() == 0);
    device.endFrame();

    return 0;
}
