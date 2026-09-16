#include <cassert>

#include "storm/render/Mesh.hpp"

int main() {
    using namespace storm::render;

    Mesh empty;
    assert(!empty.valid());
    assert(!empty.indexed());

    const BufferHandle vertex(1);
    const BufferHandle index(2);

    VertexLayout layout{};
    layout.attributeCount = 2;
    layout.stride = 28;
    layout.attributes[0] = {0, VertexFormat::Float32x3, 0};
    layout.attributes[1] = {1, VertexFormat::Float32x4, 12};
    assert(layout.valid());

    Mesh nonIndexed(MeshDesc{vertex, {}, 3, 0, IndexType::UInt16, PrimitiveTopology::Triangles, layout});
    assert(nonIndexed.valid());
    assert(!nonIndexed.indexed());
    assert(nonIndexed.drawCommand().vertexCount == 3);
    assert(!nonIndexed.drawCommand().indexed());
    assert(nonIndexed.drawCommand().vertexLayout.stride == 28);
    assert(nonIndexed.drawCommand().vertexLayout.attributeCount == 2);
    assert(nonIndexed.drawCommand().vertexLayout.attributes[1].offset == 12);

    Mesh indexed(MeshDesc{vertex, index, 8, 6, IndexType::UInt16, PrimitiveTopology::Triangles, layout});
    assert(indexed.valid());
    assert(indexed.indexed());
    assert(indexed.drawCommand().indexed());
    assert(indexed.drawCommand().vertexLayout.attributes[0].format == VertexFormat::Float32x3);

    Mesh noLayout(MeshDesc{vertex, {}, 3, 0, IndexType::UInt16, PrimitiveTopology::Triangles, {}});
    assert(!noLayout.valid());

    Mesh broken(MeshDesc{vertex, {}, 8, 6, IndexType::UInt16, PrimitiveTopology::Triangles, layout});
    assert(!broken.valid());
    assert(broken.indexed());

    return 0;
}
