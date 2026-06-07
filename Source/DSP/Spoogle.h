#pragma once
#include <cmath>

// Auto-filter resonance wobble: a Chamberlin SVF whose cutoff is swept
// by a slow LFO.  At higher amount, Q increases, giving a more
// pronounced resonant peak that moves through the spectrum.
// amount = 0..1.
class SpoogleProcessor
{
public:
    void prepare (float sr) noexcept
    {
        sampleRate = sr;
        reset();
    }

    void reset() noexcept
    {
        phase = lpL = bpL = lpR = bpR = 0.0f;
    }

    void processStereo (float& L, float& R, float amount) noexcept
    {
        if (amount < 1e-4f) return;

        // LFO: 0.7 Hz sine, sweeps cutoff 400 → 5000 Hz (log scale)
        constexpr float rateHz = 0.7f;
        phase += 6.28318530f * rateHz / sampleRate;
        if (phase >= 6.28318530f) phase -= 6.28318530f;

        const float lfo    = 0.5f * (1.0f + std::sin (phase)); // 0..1
        const float cutoff = 400.0f * std::pow (5000.0f / 400.0f, lfo);

        // Chamberlin SVF coefficients
        const float f = 2.0f * std::sin (3.14159265f * cutoff / sampleRate);
        const float Q = 0.7f + amount * 3.3f;   // 0.7 → 4.0
        const float q = 1.0f / Q;               // damping

        // Process L
        float hpL = L - lpL - q * bpL;
        bpL = f * hpL + bpL;
        lpL = f * bpL  + lpL;

        // Process R
        float hpR = R - lpR - q * bpR;
        bpR = f * hpR + bpR;
        lpR = f * bpR  + lpR;

        // Blend: mix high-Q LP (resonant peak moves with LFO) into dry signal
        const float wet = amount * 0.65f;
        L = L * (1.0f - wet) + lpL * wet;
        R = R * (1.0f - wet) + lpR * wet;
    }

private:
    float sampleRate = 44100.0f;
    float phase = 0.0f;
    float lpL = 0.0f, bpL = 0.0f;
    float lpR = 0.0f, bpR = 0.0f;
};
