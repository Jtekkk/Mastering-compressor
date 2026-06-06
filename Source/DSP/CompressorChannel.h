#pragma once
#include <cmath>
#include <algorithm>

// One compressor detection/gain-computation channel.
// Handles RMS+Peak blended detection, soft-knee gain computer,
// and program-dependent attack/release smoothing.
struct CompressorChannel
{
    // Detector state
    float rmsState  = 0.0f;
    float peakState = 0.0f;

    // Smoothed gain reduction in dB (positive = reduction)
    float grState = 0.0f;

    void reset() noexcept
    {
        rmsState = peakState = grState = 0.0f;
    }

    // Returns blended detector level (linear amplitude).
    float detectLevel (float x,
                       float rmsCoeff,
                       float peakCoeff,
                       float peakBlend) noexcept
    {
        rmsState  = rmsCoeff  * rmsState  + (1.0f - rmsCoeff)  * x * x;
        peakState = std::max (std::abs (x), peakCoeff * peakState);

        const float rms = std::sqrt (std::max (rmsState, 1e-30f));
        return rms * (1.0f - peakBlend) + peakState * peakBlend;
    }

    // Returns gain reduction in dB (positive value).
    static float computeGainReduction (float levelDb,
                                       float threshold,
                                       float ratio,
                                       float knee) noexcept
    {
        const float over = levelDb - threshold;

        if (over < -knee * 0.5f)
            return 0.0f;

        if (over > knee * 0.5f)
            return over * (1.0f - 1.0f / ratio);

        const float x = over + knee * 0.5f;
        return (x * x) / (2.0f * knee) * (1.0f - 1.0f / ratio);
    }

    // Applies attack/release smoothing with program-dependent release.
    // Returns current smoothed gain reduction in dB.
    float applySmoothing (float targetGrDb,
                          float attackCoeff,
                          float releaseMs,
                          float sampleRate) noexcept
    {
        // Program-dependent release: slower when heavily compressed
        const float grNorm = std::clamp (grState / 12.0f, 0.0f, 1.0f);
        const float adjRelease = releaseMs * (1.0f + grNorm * 4.0f);
        const float releaseCoeff = std::exp (-1.0f / (0.001f * adjRelease * sampleRate));

        if (targetGrDb > grState)
            grState = attackCoeff  * grState + (1.0f - attackCoeff)  * targetGrDb; // attack
        else
            grState = releaseCoeff * grState + (1.0f - releaseCoeff) * targetGrDb; // release

        return grState;
    }
};
