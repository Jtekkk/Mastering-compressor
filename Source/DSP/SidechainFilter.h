#pragma once
#include <cmath>

// First-order high-pass filter for sidechain detection path.
class SidechainFilter
{
public:
    void prepare (float sampleRate, float cutoffHz) noexcept
    {
        sr = sampleRate;
        setFrequency (cutoffHz);
    }

    void setFrequency (float cutoffHz) noexcept
    {
        // Bilinear-transform 1st-order HP: y[n] = a*(y[n-1] + x[n] - x[n-1])
        const float wc = 2.0f * 3.14159265f * cutoffHz / sr;
        a = 1.0f / (1.0f + wc);
        reset();
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

    float process (float x) noexcept
    {
        float y = a * (y1 + x - x1);
        x1 = x;
        y1 = y;
        return y;
    }

private:
    float sr = 44100.0f;
    float a  = 1.0f;
    float x1 = 0.0f, y1 = 0.0f;
};
