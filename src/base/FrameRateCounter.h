#pragma once

#include <vector>

class FrameRateCounter {
public:
    FrameRateCounter(size_t windowSize = 100, float smoothingFactor = 0.1f)
        : samples(windowSize, 0.0f),
        maxSamples(windowSize),
        alpha(smoothingFactor),
        index(0),
        sum(0.0f),
        smoothedFps(0.0f)
    { }

    void update(float frameTime) {
        if (frameTime <= 0.0f) return; // avoid division by zero

        float fps = 1.0f / frameTime;

        // subtract the oldest sample and replace it
        sum -= samples[index];
        samples[index] = fps;
        sum += fps;

        // move index forward
        index = (index + 1) % maxSamples;

        // average fps from the window
        float avgFps = sum / static_cast<float>(maxSamples);

        // exponential smoothing for better visual stability
        smoothedFps = (alpha * avgFps) + (1.0f - alpha) * smoothedFps;
    }

    float get() const { return smoothedFps; }

private:
    std::vector<float> samples;
    size_t maxSamples;
    float alpha;
    size_t index;
    float sum;
    float smoothedFps;
};

