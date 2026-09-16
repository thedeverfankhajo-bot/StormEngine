#include <cassert>

#include "storm/render/RenderQueue.hpp"

int main() {
    using namespace storm::render;

    RenderQueue queue;
    assert(queue.empty());
    assert(queue.size() == 0);

    DrawCommand first{};
    first.vertexBuffer = BufferHandle(1);
    first.vertexCount = 3;
    queue.submit(first);

    DrawCommand second{};
    second.topology = PrimitiveTopology::Lines;
    second.vertexBuffer = BufferHandle(2);
    second.vertexCount = 6;
    queue.submit(second);

    assert(!queue.empty());
    assert(queue.size() == 2);
    assert(queue.at(0).vertexBuffer == BufferHandle(1));
    assert(queue.at(0).vertexCount == 3);
    assert(queue.at(1).topology == PrimitiveTopology::Lines);
    assert(queue.at(1).vertexCount == 6);
    assert(queue.commands().size() == 2);

    queue.beginFrame();
    assert(queue.empty());
    assert(queue.size() == 0);

    DrawCommand indexed{};
    indexed.vertexBuffer = BufferHandle(4);
    indexed.indexBuffer = BufferHandle(5);
    indexed.indexCount = 12;
    queue.submit(indexed);
    assert(queue.size() == 1);
    assert(queue.at(0).indexed());

    return 0;
}
