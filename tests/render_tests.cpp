#include <cassert>
#include <cstdint>

#include "storm/render/RenderTypes.hpp"

int main() {
    using namespace storm::render;

    BufferHandle invalid;
    assert(!invalid.valid());

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

    assert(PrimitiveTopology::Triangles != PrimitiveTopology::Lines);
    return 0;
}
