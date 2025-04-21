#pragma once
#include <numeric>
#include <vector>

// Stores pass length in ms for each frame (indexed by frame number)
struct BenchmarkResult {
    std::vector<float> depthPrePass;
    std::vector<float> cloudComputePass;
    std::vector<float> transmittanceLUT;
    std::vector<float> multipleScatteringLUT;
    std::vector<float> skyViewLUT;
    std::vector<float> aerialPerspectiveLUT;
    std::vector<float> godRaysMask;
    std::vector<float> godRaysBlur;

    static float average(std::vector<float> &values) {
        float sum = std::accumulate(values.begin(), values.end(), 0.0f);
        return sum / values.size();
    }
};
