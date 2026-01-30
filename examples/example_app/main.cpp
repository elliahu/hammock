#include <cstdlib>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "dependency_graph.hpp"
#include "gpu_task.hpp"
#include "hammock_core.hpp"
#include "hammock_engine.hpp"
#include "hammock_renderer.hpp"

using namespace hammock;
using namespace hammock::renderer;
using namespace hammock::core;
using namespace hammock::engine;

int main() {
    auto engine = std::make_unique<Application>(RunnerMode::Editor);

    auto dependencyGraph = std::make_unique<DependencyGraph>();

    auto IMAGE_RESOURCE_HANDLE = dependencyGraph->addLogicalResource(LogicalImageResource{
        .format = ImageFormat::R16G16B16A16Sfloat,
        .type = ImageType::Type2D,
        .usage = ImageUsage::Sampled | ImageUsage::Storage,
        .width = 1920u,
        .height = 1080u,
        .persistent = true
    });

    dependencyGraph->initClearImage(IMAGE_RESOURCE_HANDLE, {1.f, 1.f, 0.f, 1.f});


    auto TARGET_RESOURCE_HANDLE = dependencyGraph->addLogicalResource(LogicalImageResource{
        .format = ImageFormat::R8G8B8A8Uint,
        .type = ImageType::Type2D,
        .usage = ImageUsage::ColorAttachment,
        .width = 1920u,
        .height = 1080u,
    });

    auto computeTask = std::make_unique<ComputeTask>("compute.comp.spv");
    auto graphicsTask = std::make_unique<GraphicsTask>("fullscreen.vert.spv", "fullscreen.frag.spv");

    computeTask->access({
        .handle = IMAGE_RESOURCE_HANDLE,
        .access = ImageAccess::StorageReadWrite,
        .binding = DescriptorBinding{0, 0, ComputeShader},
    });

    graphicsTask->access({
        .handle = IMAGE_RESOURCE_HANDLE,
        .access = ImageAccess::SampledRead,
        .binding = DescriptorBinding{0, 0, FragmentShader},
    });

    graphicsTask->access({
        .handle = TARGET_RESOURCE_HANDLE,
        .access = ImageAccess::ColorAttachmentWrite,
        .binding = AttachmentLocation{0},
    });


    auto COMPUTE_TASK_HANDLE = dependencyGraph->addTask(std::move(computeTask));
    auto GRAPHICS_TASK_HANDLE = dependencyGraph->addTask(std::move(graphicsTask));
    

    dependencyGraph->dependency(COMPUTE_TASK_HANDLE,  GRAPHICS_TASK_HANDLE, DependencyType::Debug);

    dependencyGraph->present(GRAPHICS_TASK_HANDLE, TARGET_RESOURCE_HANDLE);

    auto compiler = std::make_unique<DependencyGraphCompiler>();
    auto compiledGraph = compiler->compile(*dependencyGraph);

    engine->launch();

    return EXIT_SUCCESS;
}
