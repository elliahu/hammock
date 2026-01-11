#include <iostream>
#include <cstdlib>
#include <memory>
#include <vulkan/vulkan.hpp>

import hammock_engine;
import hammock_renderer;
import hammock_core;

using namespace hammock;
using namespace hammock::renderer;
using namespace hammock::core;
using namespace hammock::engine;

int main() {
    auto dependencyGraph = std::make_unique<DependencyGraph>();

    auto IMAGE_RESOURCE_HANDLE = dependencyGraph->addLogicalResource(LogicalImageResource{
        .format = ImageFormat::R8G8B8A8Uint
    });

    auto computeTask = std::make_unique<ComputeTask>("compute.comp.spv");
    auto graphicsTask = std::make_unique<GraphicsTask>(
        "fullscreen.vert.spv",
        "fullscreen.frag.spv"
    );


    auto storageImageSocket = std::make_unique<ImageSocket>("storageImage", IMAGE_RESOURCE_HANDLE, ImageUsage::StorageReadWrite,
                                                            DescriptorBinding{
                                                                0, 0, SocketUsageStageFlagBits::ComputeShader
                                                            });
    computeTask->addSocket(std::move(storageImageSocket));

    auto colorTargetSocket = std::make_unique<ImageSocket>("colorOutput", IMAGE_RESOURCE_HANDLE, ImageUsage::ColorAttachmentWrite,
                                                           AttachmentLocation{0});
    graphicsTask->addSocket(std::move(colorTargetSocket));


    auto pushBlock = std::make_unique<PushConstantsBlock>("pushConstants");
    auto pushField = std::make_unique<PushConstantField>("someField", PushConstantFieldType::Float);
    pushField->setFloat(420.69f);
    pushBlock->addField(std::move(pushField));
    graphicsTask->addPushConstantBlock(std::move(pushBlock));

    auto GRAPHICS_TASK_HANDLE = dependencyGraph->addTask(std::move(graphicsTask));
    auto COMPUTE_TASK_HANDLE = dependencyGraph->addTask(std::move(computeTask));

    dependencyGraph->connect(COMPUTE_TASK_HANDLE, GRAPHICS_TASK_HANDLE, DependencyType::Debug);

    auto compiler = std::make_unique<DependencyGraphCompiler>();
    auto compiledGraph = compiler->compileDependencyGraph(*dependencyGraph);


    try {
        auto engine = std::make_unique<Application>(RunnerMode::Editor);
        engine->launch();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
