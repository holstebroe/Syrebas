#ifndef SYREBAS_FILTER_HPP
#define SYREBAS_FILTER_HPP

#include <cmath>

namespace syrebas {

// Single one-pole state, used for the small coupling networks around the ladder
// (input DC-block, output bandwidth limit, resonance feedback tilt).
class OnePole {
public:
    void reset() { state_ = 0.0f; }

    // One-pole lowpass: y[n] = y[n-1] + a*(x[n]-y[n-1])
    inline float lowpass(float x, float a) {
        state_ += a * (x - state_);
        return state_;
    }

    // One-pole highpass built from the same lowpass state (x - LP(x))
    inline float highpass(float x, float a) {
        state_ += a * (x - state_);
        return x - state_;
    }

    float getState() const { return state_; }
    void setState(float s) { state_ = s; }

private:
    float state_{0.0f};
};

class Filter {
public:
    Filter();
    ~Filter() = default;

    void setSampleRate(double sampleRate);
    void reset();

    // 4x oversampled coupled diode-ladder solver (RK2/midpoint).
    float processSample(float input, float cutoffHz, float resonance);

private:
    double sampleRate_{44100.0};
    double oversampledRate_{176400.0};

    float ladderV1_{0.0f};
    float ladderV2_{0.0f};
    float ladderV3_{0.0f};
    float ladderV4_{0.0f};
    float hpFbStateX1_{0.0f};
    float hpFbStateY1_{0.0f};
    float prevInput_{0.0f};
};

} // namespace syrebas

#endif // SYREBAS_FILTER_HPP
