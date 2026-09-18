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
    assert(cache.needsApply(changed));
    cache.markApplied(changed);
    assert(!cache.needsApply(changed));
    cache.invalidate();
    assert(!cache.valid());
    assert(cache.needsApply(changed));
    return 0;
}
