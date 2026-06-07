#pragma once
#include <cmath>

// Low-frequency pump: a slow sine LFO that rhythmically dips the gain,
// creating a breathing / pumping character.  amount = 0..1.
class FlapProcessor
{
public:
    void prepare (float sr) noexcept { sampleRate = sr; reset(); }
    void reset()            noexcept { phase = 0.0f; }

    // Call once per sample; returns a linear gain multiplier.
    float nextGainMultiplier (float amount) noexcept
    {
        if (amount < 1e-4f) return 1.0f;

        constexpr float rateHz = 1.2f;
        phase += 6.28318530f * rateHz / sampleRate;
        if (phase >= 6.28318530f) phase -= 6.28318530f;

        const float lfo = 0.5f * (1.0f - std::cos (phase)); // 0 → 1
        const float dB  = -amount * 6.0f * lfo;             // 0 → -6 dB
        return std::pow (10.0f, dB / 20.0f);
    }

private:
    float sampleRate = 44100.0f;
    float phase      = 0.0f;
};
