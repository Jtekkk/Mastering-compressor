#pragma once
#include <cmath>

// Transient punch enhancer: compares fast and slow envelope followers.
// When the fast envelope exceeds the slow one (a transient), it boosts
// the signal proportionally.  amount = 0..1.
class SopankProcessor
{
public:
    void prepare (float sampleRate) noexcept
    {
        fastCoeff = std::exp (-1.0f / (0.0005f * sampleRate)); // 0.5 ms
        slowCoeff = std::exp (-1.0f / (0.025f  * sampleRate)); // 25 ms
        reset();
    }

    void reset() noexcept { fastL = fastR = slowL = slowR = 0.0f; }

    void processStereo (float& L, float& R, float amount) noexcept
    {
        if (amount < 1e-4f) return;
        L = process (L, amount, fastL, slowL);
        R = process (R, amount, fastR, slowR);
    }

private:
    float process (float x, float amount, float& fe, float& se) noexcept
    {
        const float absX = std::abs (x);
        fe = fastCoeff * fe + (1.0f - fastCoeff) * absX;
        se = slowCoeff * se + (1.0f - slowCoeff) * absX;
        const float transient = fe > se ? fe - se : 0.0f;
        return x * (1.0f + amount * 5.0f * transient);
    }

    float fastCoeff = 0.0f, slowCoeff = 0.0f;
    float fastL = 0.0f, fastR = 0.0f;
    float slowL = 0.0f, slowR = 0.0f;
};
