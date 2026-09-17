#pragma once

#include "GameClock.hpp"
#include <functional>

namespace storm {

class GameLoop final {
public:
    using FixedUpdateCallback = std::function<void(float)>;
    using UpdateCallback = std::function<void(float, float)>;

    explicit GameLoop(float fixedDeltaSeconds = 1.0f / 60.0f,
                      float maxFrameDeltaSeconds = 0.25f) noexcept
        : clock_(fixedDeltaSeconds, maxFrameDeltaSeconds) {}

    void reset() noexcept { clock_.reset(); }

    void tick(float realDeltaSeconds,
              const FixedUpdateCallback& fixedUpdate,
              const UpdateCallback& update) {
        clock_.advance(realDeltaSeconds);

        while (clock_.consumeFixedStep()) {
            if (fixedUpdate) fixedUpdate(clock_.fixedDeltaSeconds());
        }

        if (update) update(clock_.frameDeltaSeconds(), clock_.interpolationAlpha());
    }

    const GameClock& clock() const noexcept { return clock_; }
    GameClock& clock() noexcept { return clock_; }

private:
    GameClock clock_;
};

} // namespace storm
