#include <cassert>
#include <cstdint>

#include "storm/render/GlesContext.hpp"
#include "storm/render/GlesRenderDevice.hpp"

int main() {
    using namespace storm::render;

    GlesContext context;

    assert(!context.initializePbuffer(0, 1));
    assert(!context.initializePbuffer(1, 0));
    assert(!context.initializePbuffer(-1, 1));
    assert(!context.initializeWindow(nullptr));
    context.shutdown();
    assert(!context.valid());
    assert(!context.contextLost());
    context.shutdown();
    assert(!context.valid());

#if defined(__ANDROID__)
    assert(context.initializePbuffer(1, 1));
    assert(context.valid());
    assert(context.makeCurrent());
    assert(context.swap());
    assert(!context.contextLost());

    GlesRenderDevice device;
    device.onContextLost();
    device.beginFrame();
    assert(device.submittedDrawCount() == 0);
    assert(!device.submit(DrawCommand{}));
    device.endFrame();

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
