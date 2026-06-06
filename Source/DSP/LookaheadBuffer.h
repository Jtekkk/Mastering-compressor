#pragma once
#include <vector>

class LookaheadBuffer
{
public:
    void prepare (int maxDelaySamples)
    {
        bufferSize = maxDelaySamples + 2;
        buffer.assign (static_cast<size_t> (bufferSize), 0.0f);
        writePos = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    void write (float sample) noexcept
    {
        buffer[static_cast<size_t> (writePos)] = sample;
        if (++writePos >= bufferSize)
            writePos = 0;
    }

    // delaySamples == 0 returns the sample just written
    float read (int delaySamples) const noexcept
    {
        int pos = writePos - delaySamples - 1;
        if (pos < 0) pos += bufferSize;
        return buffer[static_cast<size_t> (pos)];
    }

private:
    std::vector<float> buffer;
    int bufferSize = 2;
    int writePos   = 0;
};
