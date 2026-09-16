#include <cassert>

#include "storm/render/Mesh.hpp"

int main() {
    using namespace storm::render;

    Mesh empty;
    assert(!empty.valid());
    assert(!empty.indexed());

    const BufferHandle vertex(1);
    const BufferHandle index(2);

    Mesh nonIndexed(MeshDesc{vertex, {}, 3, 0, IndexType::UInt16, PrimitiveTopology::Triangles});
    assert(nonIndexed.valid());
    assert(!nonIndexed.indexed());
    assert(nonIndexed.drawCommand().vertexCount == 3);
    assert(!nonIndexed.drawCommand().indexed());

    Mesh indexed(MeshDesc{vertex, index, 8, 6, IndexType::UInt16, PrimitiveTopology::Triangles});
    assert(indexed.valid());
    assert(indexed.indexed());
    assert(indexed.drawCommand().indexed());

    Mesh broken(MeshDesc{vertex, {}, 8, 6, IndexType::UInt16, PrimitiveTopology::Triangles});
    assert(!broken.valid());
    assert(broken.indexed());

    return 0;
}
