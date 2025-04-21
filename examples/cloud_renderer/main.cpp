#include <iostream>
#include <cstdint>
#include <hammock/hammock.h>

#include "scenes/ParticipatingMediumScene.h"

#include "renderer/Renderer.h"

// To use reprojection in the cloud rendering, define REPROJECTION macro here and in the shaders/clouds.slang shader
//#define REPROJECTION

using namespace hammock;

int main(int argc, char * argv[]) {
    ArgParser parser;
    parser.addArgument<int32_t>("width", "Window width in pixels");
    parser.addArgument<int32_t>("height", "Window height in pixels");
    parser.addArgument<std::string>("scene", "Scene option: [noise, clouds, medium, renderer]");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception & e) {
        Logger::log(LOG_LEVEL_ERROR, e.what());
    }

    const auto width = parser.get<int32_t>("width");
    const auto height = parser.get<int32_t>("height");
    auto selectedScene = parser.get<std::string>("scene");

    if (selectedScene == "medium") {
        ParticipatingMediumScene mediumScene{"Participating medium playground", static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        mediumScene.render();
    }
    else if (selectedScene == "renderer") {
        Renderer renderer{width, height};
        renderer.render();
        auto benchmarkResult = renderer.getBenchmarkResult();
        std::cout << "Depth pre-pass avg: " << BenchmarkResult::average(benchmarkResult.depthPrePass) << std::endl;
        std::cout << "Cloud compute avg: " << BenchmarkResult::average(benchmarkResult.cloudComputePass) << std::endl;
        std::cout << "Composition avg: " << BenchmarkResult::average(benchmarkResult.compositionPass) << std::endl;
    }
    else {
        Logger::log(LOG_LEVEL_ERROR, "Invalid scene option");
    }
}