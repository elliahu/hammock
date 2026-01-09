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
    auto taskGraph = std::make_unique<TaskGraph>();

    auto computeTask = std::make_unique<ComputeTask>("compute.comp.spv");
    auto graphicsTask = std::make_unique<GraphicsTask>(
        "fullscreen.vert.spv",
        "fullscreen.frag.spv"
    );


    auto storageImageSocket = std::make_unique<ImageSocket>("storageImage", ImageUsage::StorageReadWrite,
                                                            ImageType::Type2D, DescriptorBinding{
                                                                0, 0, SocketUsageStageFlagBits::ComputeShader
                                                            });
    auto STORAGE_IMAGE_HANDLE = computeTask->addSocket(std::move(storageImageSocket));

    auto colorTargetSocket = std::make_unique<ImageSocket>("colorOutput", ImageUsage::ColorAttachmentWrite,
                                                           ImageType::Type2D,
                                                           AttachmentLocation{0});
    auto COLOR_TARGET_HANDLE = graphicsTask->addSocket(std::move(colorTargetSocket));


    auto pushBlock = std::make_unique<PushConstantsBlock>("pushConstants");
    auto pushField = std::make_unique<PushConstantField>("someField", PushConstantFieldType::Float);
    pushField->setFloat(420.69f);
    pushBlock->addField(std::move(pushField));
    graphicsTask->addPushConstantBlock(std::move(pushBlock));

    auto GRAPHICS_TASK_HANDLE = taskGraph->add(std::move(graphicsTask));
    auto COMPUTE_TASK_HANDLE = taskGraph->add(std::move(computeTask));

    taskGraph->connect(COMPUTE_TASK_HANDLE, STORAGE_IMAGE_HANDLE, GRAPHICS_TASK_HANDLE, COLOR_TARGET_HANDLE);


    try {
        auto engine = std::make_unique<Application>(RunnerMode::Editor);
        engine->launch();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
