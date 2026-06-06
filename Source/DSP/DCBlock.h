#pragma once

class DCBlock
{
public:
    void reset() noexcept { x1 = y1 = 0.0f; }

    float process (float x) noexcept
    {
        float y = x - x1 + 0.9999f * y1;
        x1 = x;
        y1 = y;
        return y;
    }

private:
    float x1 = 0.0f, y1 = 0.0f;
};
