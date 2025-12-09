#include <hammock/hammock.h>

#include <cstdlib>
#include <memory>

#include "hammock/rendergraph/FrameGraph.h"
#include "hammock/rendergraph/Pass.h"

using namespace Hammock;
using namespace Rendergraph;

auto getCompiledShaderPath(const std::string& name) -> std::string {
    return std::string(HAMMOCK_BUILD_DIR) + "/spv/" + name;
}

auto main() -> int {
    VulkanInstance instance{};
    Hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};

    ResourceManager::initialize(device);
    FrameManager::initialize(window, device);
    DescriptorPool::initialize(device,
        1000,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000},
        });

    Scoped deleter([] {
        ResourceManager::dispose();
        FrameManager::dispose();
        DescriptorPool::dispose();
    });

    // Create the rendergraph instance
    auto graph = std::make_unique<FrameGraph>(device);

    auto outColor = graph->importSwapChainImage();

    auto vBuffer = graph->createImage("vBuffer", ImageDesc::StorageRgba16F(1920, 1080));

    auto compute = ComputePass::create("COMPUTE1");
    compute->storageImage({vBuffer, VK_SHADER_STAGE_COMPUTE_BIT});
    compute->binding({{vBuffer, 0}});
    compute->computeShader({.spv = Filesystem::readFile(getCompiledShaderPath("compute.comp.spv"))});
    compute->dispatch({{10, 10, 0}});

    auto graphics = GraphicsPass::create("GRAPHICS1");
    graphics->combinedImageSampler({vBuffer, VK_SHADER_STAGE_ALL_GRAPHICS});
    graphics->binding({{vBuffer, 0}});
    graphics->colorTarget({outColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE});

    graph->addPass(compute);
    graph->addPass(graphics);

    // Build the render graph
    try{
        graph->build();
    }
    catch(const std::exception& error){
        Logger::error("Rendergraph build failed: %s", error.what());
        exit(EXIT_FAILURE);
    }

    graph->dumpDotfile("graph.dot");


    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();

        graph->execute();
    }
    device.waitIdle();

    return EXIT_SUCCESS;
}
