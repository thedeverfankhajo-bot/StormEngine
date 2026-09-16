#pragma once

#include <algorithm>
#include <cmath>

namespace storm {

class GameClock final {
public:
    explicit GameClock(float fixedDeltaSeconds = 1.0f / 60.0f,
                       float maxFrameDeltaSeconds = 0.25f) noexcept
        : fixedDeltaSeconds_(fixedDeltaSeconds),
          maxFrameDeltaSeconds_(maxFrameDeltaSeconds) {}

    void reset() noexcept {
        frameDeltaSeconds_ = 0.0f;
        accumulatorSeconds_ = 0.0f;
        fixedSteps_ = 0;
    }

    void advance(float realDeltaSeconds) noexcept {
        frameDeltaSeconds_ = std::clamp(realDeltaSeconds, 0.0f, maxFrameDeltaSeconds_);
        accumulatorSeconds_ += frameDeltaSeconds_;
        fixedSteps_ = static_cast<unsigned int>(
            std::floor(accumulatorSeconds_ / fixedDeltaSeconds_));
    }

    bool consumeFixedStep() noexcept {
        if (fixedSteps_ == 0) return false;
        accumulatorSeconds_ -= fixedDeltaSeconds_;
        --fixedSteps_;
        return true;
    }

    float frameDeltaSeconds() const noexcept { return frameDeltaSeconds_; }
    float fixedDeltaSeconds() const noexcept { return fixedDeltaSeconds_; }
    float accumulatorSeconds() const noexcept { return accumulatorSeconds_; }
    unsigned int pendingFixedSteps() const noexcept { return fixedSteps_; }

    float interpolationAlpha() const noexcept {
        return fixedDeltaSeconds_ > 0.0f
            ? accumulatorSeconds_ / fixedDeltaSeconds_
            : 0.0f;
    }

private:
    float fixedDeltaSeconds_;
    float maxFrameDeltaSeconds_;
    float frameDeltaSeconds_{0.0f};
    float accumulatorSeconds_{0.0f};
    unsigned int fixedSteps_{0};
};

} // namespace storm
