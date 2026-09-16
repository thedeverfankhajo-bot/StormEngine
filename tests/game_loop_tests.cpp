#include <cassert>
#include <vector>

#include "storm/core/GameClock.hpp"
#include "storm/core/GameLoop.hpp"

namespace {

void testFixedStepAccumulation() {
    storm::GameClock clock{0.1f, 1.0f};
    clock.advance(0.25f);

    assert(clock.pendingFixedSteps() == 2);
    assert(clock.accumulatorSeconds() > 0.149f);
    assert(clock.accumulatorSeconds() < 0.151f);

    assert(clock.consumeFixedStep());
    assert(clock.consumeFixedStep());
    assert(!clock.consumeFixedStep());
}

void testFrameDeltaClamp() {
    storm::GameClock clock{0.1f, 0.2f};
    clock.advance(1.0f);

    assert(clock.frameDeltaSeconds() == 0.2f);
    assert(clock.pendingFixedSteps() == 2);
}

void testGameLoopOrder() {
    storm::GameLoop loop{0.1f, 1.0f};
    std::vector<int> events;
    float frameDelta = 0.0f;
    float alpha = 0.0f;

    loop.tick(0.25f,
        [&](float fixedDelta) {
            assert(fixedDelta == 0.1f);
            events.push_back(1);
        },
        [&](float delta, float interpolation) {
            frameDelta = delta;
            alpha = interpolation;
            events.push_back(2);
        });

    assert((events == std::vector<int>{1, 1, 2}));
    assert(frameDelta > 0.249f && frameDelta < 0.251f);
    assert(alpha > 0.49f && alpha < 0.51f);
}

} // namespace

int main() {
    testFixedStepAccumulation();
    testFrameDeltaClamp();
    testGameLoopOrder();
    return 0;
}
