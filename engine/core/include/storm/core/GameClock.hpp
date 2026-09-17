#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace storm {

class GameClock final {
public:
    explicit GameClock(float fixedDeltaSeconds = 1.0f / 60.0f,
                       float maxFrameDeltaSeconds = 0.25f) noexcept
        : fixedDeltaSeconds_(sanitizeFixedDelta(fixedDeltaSeconds)),
          maxFrameDeltaSeconds_(sanitizeMaxFrameDelta(maxFrameDeltaSeconds)) {}

    void reset() noexcept {
        frameDeltaSeconds_ = 0.0f;
        accumulatorSeconds_ = 0.0f;
        fixedSteps_ = 0;
    }

    void advance(float realDeltaSeconds) noexcept {
        if (!std::isfinite(realDeltaSeconds)) {
            realDeltaSeconds = 0.0f;
        }
        frameDeltaSeconds_ = std::clamp(realDeltaSeconds, 0.0f, maxFrameDeltaSeconds_);
        accumulatorSeconds_ += frameDeltaSeconds_;

        if (fixedDeltaSeconds_ <= 0.0f) {
            fixedSteps_ = 0;
            accumulatorSeconds_ = 0.0f;
            return;
        }

        const float stepCount = std::floor(accumulatorSeconds_ / fixedDeltaSeconds_);
        if (!std::isfinite(stepCount) || stepCount <= 0.0f) return;

        const auto maxSteps = static_cast<float>(std::numeric_limits<unsigned int>::max());
        if (stepCount >= maxSteps) {
            fixedSteps_ = std::numeric_limits<unsigned int>::max();
            accumulatorSeconds_ = 0.0f;
            return;
        }

        const auto steps = static_cast<unsigned int>(stepCount);
        const auto remaining = accumulatorSeconds_ - static_cast<float>(steps) * fixedDeltaSeconds_;
        fixedSteps_ = fixedSteps_ > std::numeric_limits<unsigned int>::max() - steps
            ? std::numeric_limits<unsigned int>::max()
            : fixedSteps_ + steps;
        accumulatorSeconds_ = std::max(0.0f, remaining);
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
    static float sanitizeFixedDelta(float value) noexcept {
        if (!std::isfinite(value) || value <= 0.0f) return 1.0f / 60.0f;
        return value;
    }

    static float sanitizeMaxFrameDelta(float value) noexcept {
        if (!std::isfinite(value) || value <= 0.0f) return 0.25f;
        return value;
    }

    float fixedDeltaSeconds_;
    float maxFrameDeltaSeconds_;
    float frameDeltaSeconds_{0.0f};
    float accumulatorSeconds_{0.0f};
    unsigned int fixedSteps_{0};
};

} // namespace storm
