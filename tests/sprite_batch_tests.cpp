#include <cassert>
#include <limits>

#include "storm/render/SpriteBatch.hpp"

int main() {
    storm::render::SpriteBatch batch{2};
    assert(batch.spriteCount() == 0);
    assert(batch.add({1.0f, 2.0f}, {4.0f, 3.0f}));
    assert(batch.add({-2.0f, 5.0f}, {2.0f, 2.0f}, {0.25f, 0.5f}, {0.75f, 1.0f}));
    assert(batch.spriteCount() == 2);

    const auto geometry = batch.build();
    assert(geometry.valid());
    assert(geometry.vertices.size() == 8);
    assert(geometry.indices.size() == 12);
    assert(geometry.indices[0] == 0 && geometry.indices[1] == 1 && geometry.indices[2] == 2);
    assert(geometry.indices[6] == 4 && geometry.indices[11] == 7);
    assert(geometry.vertices[0].position.x == 1.0f);
    assert(geometry.vertices[2].position.x == 5.0f);
    assert(geometry.vertices[4].uv.x == 0.25f);
    assert(geometry.vertices[6].uv.y == 1.0f);

    batch.clear();
    assert(batch.spriteCount() == 0);
    assert(!batch.add({std::numeric_limits<float>::quiet_NaN(), 0.0f}, {1.0f, 1.0f}));
    assert(!batch.add({0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f},
                      {std::numeric_limits<float>::infinity(), 1.0f}));
    assert(!batch.add({std::numeric_limits<float>::max(), 0.0f}, {std::numeric_limits<float>::max(), 0.0f}));
    assert(batch.vertices().empty());
    assert(batch.indices().empty());
    return 0;
}
