#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "storm/render/RenderTypes.hpp"

int main() {
    using namespace storm::render;

    static_assert(std::is_trivially_copyable_v<DrawCommand>);

    BufferHandle invalid;
    assert(!invalid.valid());
    assert(invalid.id() == BufferHandle::invalidId);

    BufferHandle a(7);
    BufferHandle b(7);
    BufferHandle c(8);
    assert(a.valid());
    assert(a.id() == 7);
    assert(a == b);
    assert(a != c);
    assert(BufferHandle(BufferHandle::invalidId).valid() == false);

    TextureHandle texture(3);
    ShaderHandle shader(4);
    MaterialHandle material(5);
    assert(texture.valid() && texture.id() == 3);
    assert(shader.valid() && shader.id() == 4);
    assert(material.valid() && material.id() == 5);

    // Handles are strongly typed: different resource kinds cannot be compared.
    assert(BufferHandle(3) != BufferHandle(4));
    assert(TextureHandle(3) == TextureHandle(3));

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

    assert(PrimitiveTopology::Triangles != PrimitiveTopology::Lines);
    assert(IndexType::UInt16 != IndexType::UInt32);

    DrawCommand nonIndexed{};
    assert(nonIndexed.topology == PrimitiveTopology::Triangles);
    assert(!nonIndexed.indexed());
    nonIndexed.vertexBuffer = BufferHandle(11);
    nonIndexed.vertexCount = 36;
    nonIndexed.firstVertex = 2;
    assert(nonIndexed.vertexBuffer.valid());
    assert(nonIndexed.vertexCount == 36);
    assert(nonIndexed.firstVertex == 2);
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
    assert(indexed.topology == PrimitiveTopology::TriangleStrip);
    assert(indexed.indexCount == 96);
    assert(indexed.firstIndex == 4);
    assert(indexed.baseVertex == -3);
    assert(indexed.indexType == IndexType::UInt16);

    indexed.indexCount = 0;
    assert(!indexed.indexed());
    indexed.indexCount = 1;
    indexed.indexBuffer = BufferHandle{};
    assert(!indexed.indexed());

    DrawCommand copy = nonIndexed;
    assert(copy.vertexBuffer == nonIndexed.vertexBuffer);
    assert(copy.vertexCount == nonIndexed.vertexCount);

    return 0;
}
