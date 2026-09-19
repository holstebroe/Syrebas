#include "Filter.hpp"
#include <cmath>
#include <algorithm>

namespace syrebas {

namespace {

// Solve a 4x4 tridiagonal system via the Thomas algorithm:
//   row0: a[0]*x0 + bUp[0]*x1                                   = d[0]
//   row1: cLow[0]*x0 + a[1]*x1 + bUp[1]*x2                      = d[1]
//   row2:              cLow[1]*x1 + a[2]*x2 + bUp[2]*x3         = d[2]
//   row3:                           cLow[2]*x2 + a[3]*x3        = d[3]
// `a` and `d` are modified in place (forward elimination); results land in x.
inline void solveTridiagonal4(float a[4], const float bUp[3], const float cLow[3],
                               float d[4], float x[4]) {
    for (int i = 1; i < 4; ++i) {
        float w = cLow[i - 1] / a[i - 1];
        a[i] -= w * bUp[i - 1];
        d[i] -= w * d[i - 1];
    }
    x[3] = d[3] / a[3];
    for (int i = 2; i >= 0; --i) {
        x[i] = (d[i] - bUp[i] * x[i + 1]) / a[i];
    }
}

} // namespace

Filter::Filter() {
    setSampleRate(44100.0);
}

void Filter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    oversampledRate_ = sampleRate_ * 4.0;
    reset();
}

void Filter::reset() {
    ladderV1_ = ladderV2_ = ladderV3_ = ladderV4_ = 0.0f;
    hpFbStateX1_ = hpFbStateY1_ = 0.0f;
    prevInput_ = 0.0f;
}

float Filter::processSample(float input, float cutoffHz, float resonance) {
    // 4x oversampling step for coupled diode ladder
    float dt = 1.0f / static_cast<float>(oversampledRate_);
    float totalCutoffHz = std::min(std::max(cutoffHz, 20.0f), 18000.0f);

    float wc = 2.0f * 3.14159265358979323846f * totalCutoffHz;

    // High pass in feedback path: cutoff dynamically scales between 150 Hz and 250 Hz (Section 13)
    float resNorm = std::min(std::max(resonance, 0.0f), 1.0f);
    float hpfCutoff = 150.0f + 100.0f * resNorm;
    float hpfAlpha = 1.0f / (1.0f + 2.0f * 3.14159265358979323846f * hpfCutoff * dt);

    // Coupled 4-stage diode ladder oscillation threshold k = 33.0 for self-oscillation & intense squelch
    float kFb = resNorm * 33.0f;

    // Physical BJT thermal voltage V_T = 26mV. Effective scale factor Vt = 2*V_T = 0.052V (Vt_inv = 1 / 0.052 = 19.23)
    const float Vt = 0.052f;
    const float Vt_inv = 19.23f;

    float filterOut = 0.0f;

    float prevIn = prevInput_;
    prevInput_ = input;

    for (int os = 0; os < 4; ++os) {
        // Linear interpolation across 4x oversampling sub-steps
        float alphaOS = static_cast<float>(os + 1) / 4.0f;
        float currIn = prevIn + alphaOS * (input - prevIn);

        // Physical input signal voltage entering the ladder buffer (~0.05V RMS)
        float inSample = currIn * 0.05f;

        // Feedback calculation (hpOut is in volts matching ladderV4_)
        float hpOut = hpfAlpha * (hpFbStateY1_ + ladderV4_ - hpFbStateX1_);
        hpFbStateX1_ = ladderV4_;
        hpFbStateY1_ = hpOut;

        float u = inSample - hpOut * kFb;

        // Coupled Diode Ladder Differential Equations with physical BJT differential pair scaling (Section 7, 84)
        // dv1/dt = w * Vt * [ tanh((u - v1)/Vt) - tanh((v1 - v2)/Vt) ]
        // dv2/dt = w * Vt * [ tanh((v1 - v2)/Vt) - tanh((v2 - v3)/Vt) ]
        // dv3/dt = w * Vt * [ tanh((v2 - v3)/Vt) - tanh((v3 - v4)/Vt) ]
        // dv4/dt = 2w * Vt * tanh((v3 - v4)/Vt)
        float h = dt;
        float v1 = ladderV1_;
        float v2 = ladderV2_;
        float v3 = ladderV3_;
        float v4 = ladderV4_;

        // K1
        float dv1_1 = wc * Vt * (std::tanh((u - v1) * Vt_inv) - std::tanh((v1 - v2) * Vt_inv));
        float dv2_1 = wc * Vt * (std::tanh((v1 - v2) * Vt_inv) - std::tanh((v2 - v3) * Vt_inv));
        float dv3_1 = wc * Vt * (std::tanh((v2 - v3) * Vt_inv) - std::tanh((v3 - v4) * Vt_inv));
        float dv4_1 = 2.0f * wc * Vt * std::tanh((v3 - v4) * Vt_inv);

        // K2
        float v1_mid = v1 + 0.5f * h * dv1_1;
        float v2_mid = v2 + 0.5f * h * dv2_1;
        float v3_mid = v3 + 0.5f * h * dv3_1;
        float v4_mid = v4 + 0.5f * h * dv4_1;

        float hpOut_mid = hpfAlpha * (hpFbStateY1_ + v4_mid - hpFbStateX1_);
        float u_mid = inSample - hpOut_mid * kFb;

        float dv1_2 = wc * Vt * (std::tanh((u_mid - v1_mid) * Vt_inv) - std::tanh((v1_mid - v2_mid) * Vt_inv));
        float dv2_2 = wc * Vt * (std::tanh((v1_mid - v2_mid) * Vt_inv) - std::tanh((v2_mid - v3_mid) * Vt_inv));
        float dv3_2 = wc * Vt * (std::tanh((v2_mid - v3_mid) * Vt_inv) - std::tanh((v3_mid - v4_mid) * Vt_inv));
        float dv4_2 = 2.0f * wc * Vt * std::tanh((v3_mid - v4_mid) * Vt_inv);

        ladderV1_ += h * dv1_2;
        ladderV2_ += h * dv2_2;
        ladderV3_ += h * dv3_2;
        ladderV4_ += h * dv4_2;

        filterOut += (ladderV4_ / 0.05f) * 0.25f; // Normalize voltage and average 4x decimation
    }

    return filterOut;
}


} // namespace syrebas
