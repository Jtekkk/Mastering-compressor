#pragma once
#include <cmath>
#include <algorithm>

// Brick-wall true-peak limiter. Must be called inside the oversampled domain
// so that inter-sample peaks are visible.
class TruePeakLimiter
{
public:
    void prepare (float sampleRate) noexcept
    {
        // ~2 ms release at the oversampled rate
        releaseCoeff = std::exp (-1.0f / (0.002f * sampleRate));
        reset();
    }

    void reset() noexcept { gainL = gainR = 1.0f; }

    void setCeiling (float linearCeiling) noexcept { ceiling = linearCeiling; }

    void processStereo (float& L, float& R) noexcept
    {
        // Fast lookahead-less gain computer (peak hold)
        const float absL = std::abs (L);
        const float absR = std::abs (R);

        float targetGainL = (absL > ceiling) ? ceiling / absL : 1.0f;
        float targetGainR = (absR > ceiling) ? ceiling / absR : 1.0f;

        // Instantaneous attack
        gainL = std::min (gainL, targetGainL);
        gainR = std::min (gainR, targetGainR);

        L *= gainL;
        R *= gainR;

        // Release
        gainL = releaseCoeff * gainL + (1.0f - releaseCoeff);
        gainR = releaseCoeff * gainR + (1.0f - releaseCoeff);
    }

    // Returns max of last processed L/R peak (post-limit)
    float getTruePeakDb() const noexcept
    {
        const float peak = std::max (std::abs (lastL), std::abs (lastR));
        return peak > 1e-7f ? 20.0f * std::log10 (peak) : -144.0f;
    }

private:
    float releaseCoeff = 0.999f;
    float ceiling = 1.0f;
    float gainL   = 1.0f;
    float gainR   = 1.0f;
    float lastL   = 0.0f;
    float lastR   = 0.0f;
};
