#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "storm/render/RenderQueue.hpp"
#include "storm/render/RenderTypes.hpp"

int main() {
    using namespace storm::render;

    static_assert(std::is_trivially_copyable_v<DrawCommand>);
    static_assert(std::is_trivially_copyable_v<BufferHandle>);
    static_assert(sizeof(BufferHandle) == sizeof(std::uint32_t));

    BufferHandle invalid;
    assert(!invalid.valid());
    assert(invalid.id() == BufferHandle::invalidId);
    assert(BufferHandle(BufferHandle::invalidId).valid() == false);

    BufferHandle a(7);
    BufferHandle b(7);
    BufferHandle c(8);
    assert(a.valid());
    assert(a.id() == 7);
    assert(a == b);
    assert(a != c);

    TextureHandle texture(3);
    ShaderHandle shader(4);
    MaterialHandle material(5);
    assert(texture.valid() && texture.id() == 3);
    assert(shader.valid() && shader.id() == 4);
    assert(material.valid() && material.id() == 5);

    BufferDesc buffer{};
    assert(buffer.size == 0);
    assert(buffer.usage == BufferUsage::Static);
    buffer.size = 4096;
    buffer.usage = BufferUsage::Dynamic;
    assert(buffer.size == 4096);
    assert(buffer.usage == BufferUsage::Dynamic);

    TextureDesc textureDesc{};
    assert(textureDesc.width == 1);
    assert(textureDesc.height == 1);
    assert(textureDesc.mipLevels == 1);
    textureDesc.width = 128;
    textureDesc.height = 64;
    textureDesc.mipLevels = 4;
    assert(textureDesc.width == 128);
    assert(textureDesc.height == 64);
    assert(textureDesc.mipLevels == 4);

    DrawCommand nonIndexed{};
    nonIndexed.vertexBuffer = BufferHandle(11);
    nonIndexed.vertexCount = 36;
    nonIndexed.firstVertex = 2;
    assert(!nonIndexed.indexed());

    DrawCommand indexed{};
    indexed.topology = PrimitiveTopology::TriangleStrip;
    indexed.vertexBuffer = BufferHandle(12);
    indexed.indexBuffer = BufferHandle(13);
    indexed.indexCount = 96;
    indexed.firstIndex = 4;
    indexed.baseVertex = -3;
    indexed.indexType = IndexType::UInt16;
    assert(indexed.indexed());

    indexed.indexCount = 0;
    assert(!indexed.indexed());
    indexed.indexCount = 1;
    indexed.indexBuffer = BufferHandle{};
    assert(!indexed.indexed());

    RenderQueue queue;
    assert(queue.empty());
    assert(queue.size() == 0);

    queue.submit(nonIndexed);
    queue.submit(indexed);
    assert(!queue.empty());
    assert(queue.size() == 2);
    assert(queue.at(0).vertexCount == 36);
    assert(queue.at(1).indexCount == 1);

    const auto& commands = queue.commands();
    assert(commands.size() == 2);
    assert(commands[0].vertexBuffer == BufferHandle(11));
    assert(commands[1].vertexBuffer == BufferHandle(12));

    queue.beginFrame();
    assert(queue.empty());
    assert(queue.size() == 0);

    // A queue can be reused for another frame without retaining old commands.
    queue.submit(indexed);
    assert(queue.size() == 1);
    assert(queue.at(0).indexType == IndexType::UInt16);

    return 0;
}
