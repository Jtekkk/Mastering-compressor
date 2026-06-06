#pragma once
#include <cmath>

enum class HarmonicMode { None = 0, Tube, Tape, Transformer };

// Harmonic saturation stage — processes one stereo pair in-place.
// Must be called in the oversampled domain.
class HarmonicSaturation
{
public:
    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        // Tape low-pass at ~8 kHz
        const float wc = 2.0f * 3.14159265f * 8000.0f / sampleRate;
        lpCoeff = std::exp (-wc);
        reset();
    }

    void reset() noexcept { lpStateL = lpStateR = 0.0f; }

    void setMode  (HarmonicMode m) noexcept { mode  = m; }
    void setDrive (float d)        noexcept { drive = d; } // 0..1

    void processStereo (float& L, float& R) noexcept
    {
        switch (mode)
        {
            case HarmonicMode::None:        break;
            case HarmonicMode::Tube:        L = tube (L); R = tube (R); break;
            case HarmonicMode::Tape:        L = tape (L, lpStateL); R = tape (R, lpStateR); break;
            case HarmonicMode::Transformer: L = transformer (L); R = transformer (R); break;
        }
    }

private:
    float tube (float x) const noexcept
    {
        const float d = 1.0f + drive * 3.0f;  // drive up to 4×
        return std::tanh (x * d) / std::tanh (d); // normalised
    }

    float tape (float x, float& lp) const noexcept
    {
        const float d = 1.0f + drive * 2.4f;
        float y = std::tanh (x * d * 0.8f);
        lp = lpCoeff * lp + (1.0f - lpCoeff) * y;
        return lp / std::tanh (d * 0.8f);
    }

    float transformer (float x) const noexcept
    {
        const float d = 1.0f + drive * 3.6f;
        float y = std::tanh (x * d * 1.2f);
        y += x * x * x * 0.05f;               // odd-harmonic character
        return y / std::tanh (d * 1.2f);
    }

    HarmonicMode mode = HarmonicMode::None;
    float drive  = 0.3f;
    float sr     = 44100.0f;
    float lpCoeff    = 0.0f;
    float lpStateL   = 0.0f;
    float lpStateR   = 0.0f;
};
