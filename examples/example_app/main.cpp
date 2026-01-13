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
    auto engine = std::make_unique<Application>(RunnerMode::Editor);

    auto dependencyGraph = std::make_unique<DependencyGraph>();

    auto IMAGE_RESOURCE_HANDLE = dependencyGraph->addLogicalResource(LogicalImageResource{
        .format = ImageFormat::R8G8B8A8Uint,
        .type = ImageType::Type2D,
        .usage = ImageUsage::Sampled | ImageUsage::ColorAttachment,
        .width = 1920u,
        .height = 1080u,
    });

    auto computeTask = std::make_unique<ComputeTask>("compute.comp.spv");
    auto graphicsTask = std::make_unique<GraphicsTask>(
        "fullscreen.vert.spv",
        "fullscreen.frag.spv"
    );


    auto storageImageSocket = std::make_unique<ImageSocket>("storageImage", IMAGE_RESOURCE_HANDLE, ImageAccess::SampledRead,
                                                            DescriptorBinding{
                                                                0, 0, ComputeShader
                                                            });
    computeTask->addSocket(std::move(storageImageSocket));

    auto colorTargetSocket = std::make_unique<ImageSocket>("colorOutput", IMAGE_RESOURCE_HANDLE, ImageAccess::ColorAttachmentWrite,
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



    engine->launch();


    return EXIT_SUCCESS;
}
