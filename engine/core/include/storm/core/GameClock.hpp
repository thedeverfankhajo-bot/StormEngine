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

        if (fixedDeltaSeconds_ <= 0.0f) {
            fixedSteps_ = 0;
            accumulatorSeconds_ = 0.0f;
            return;
        }

        const auto steps = static_cast<unsigned int>(
            std::floor(accumulatorSeconds_ / fixedDeltaSeconds_));
        fixedSteps_ += steps;
        accumulatorSeconds_ -= static_cast<float>(steps) * fixedDeltaSeconds_;
    }

    bool consumeFixedStep() noexcept {
        if (fixedSteps_ == 0) return false;
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
