#include <cassert>
#include "storm/render/RenderStateCache.hpp"
int main() {
    storm::render::RenderStateCache cache;
    storm::render::RenderState state{};
    assert(!cache.valid());
    assert(cache.needsApply(state));
    cache.markApplied(state);
    assert(cache.valid());
    assert(!cache.needsApply(state));
    auto changed = state;
    changed.blendEnabled = true;
    changed.scissorEnabled = true;
    changed.scissorX = -4;
    changed.scissorY = 8;
    changed.scissorWidth = 320;
    changed.scissorHeight = 180;
    assert(cache.needsApply(changed));
    cache.markApplied(changed);
    assert(!cache.needsApply(changed));
    assert(cache.state().scissorX == -4);
    assert(cache.state().scissorWidth == 320);
    cache.invalidate();
    assert(!cache.valid());
    assert(cache.needsApply(changed));
    return 0;
}
