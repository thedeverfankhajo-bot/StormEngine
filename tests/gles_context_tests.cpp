#include <cassert>

#include "storm/render/GlesContext.hpp"

int main() {
    storm::render::GlesContext context;

    assert(!context.initializePbuffer(0, 1));
    assert(!context.initializePbuffer(1, 0));
    assert(!context.initializePbuffer(-1, 1));
    assert(!context.initializeWindow(nullptr));

#if defined(__ANDROID__)
    assert(context.initializePbuffer(1, 1));
    assert(context.valid());
    assert(context.makeCurrent());
    assert(context.swap());
    assert(!context.contextLost());
    context.shutdown();
    assert(!context.valid());
    assert(!context.contextLost());
#else
    assert(!context.initializePbuffer(1, 1));
    assert(!context.valid());
    assert(!context.contextLost());
    assert(!context.makeCurrent());
    assert(!context.swap());
#endif

    return 0;
}
