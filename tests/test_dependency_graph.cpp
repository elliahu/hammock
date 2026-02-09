#include <gtest/gtest.h>

#include "hammock_dependency_graph.hpp"
#include "hammock_engine.hpp"
#include "hammock_renderer.hpp"
#include "hammock_core.hpp"

using namespace hammock;
using namespace hammock::renderer;
using namespace hammock::core;
using namespace hammock::engine;

TEST(DependencyGraphTests, GraphCompilationDoesNotThrow) {
    EXPECT_NO_THROW([&]() {
        auto engine = std::make_unique<Application>(RunnerMode::Editor);

        auto dependencyGraph = std::make_unique<DependencyGraph>();

        auto IMAGE_RESOURCE_HANDLE = dependencyGraph->image(LogicalImageResource{
            .format = ImageFormat::R16G16B16A16Sfloat,
            .type = ImageType::Type2D,
            .usage = ImageUsage::Sampled | ImageUsage::Storage,
            .width = 1920u,
            .height = 1080u,
        });

        dependencyGraph->initClearImage(IMAGE_RESOURCE_HANDLE, {1.f, 1.f, 0.f, 1.f});

        auto BUFFER_RESOURCE_HANDLE =
            dependencyGraph->buffer(LogicalBufferResource{.type = BufferType::DeviceOnly,
                .usage = BufferUsage::StorageBuffer,
                .instanceSize = sizeof(math::Vec3),
                .instanceCount = 10u});

        auto TARGET_RESOURCE_HANDLE =
            dependencyGraph->image(LogicalImageResource{.format = ImageFormat::R8G8B8A8Uint,
                .type = ImageType::Type2D,
                .usage = ImageUsage::ColorAttachment,
                .width = 1920u,
                .height = 1080u,
                .persistent = false});

        auto computeTask1 = std::make_unique<ComputeTask>("compute1.comp.spv");
        auto computeTask2 = std::make_unique<ComputeTask>("compute2.comp.spv");
        auto graphicsTask = std::make_unique<GraphicsTask>("fullscreen.vert.spv", "fullscreen.frag.spv");

        computeTask1->access({
            .handle = IMAGE_RESOURCE_HANDLE,
            .access = ImageAccess::StorageReadWrite,
            .binding = DescriptorBinding{0, 0, DescriptorUsageStage::ComputeShader},
        });

        computeTask2->access({.handle = BUFFER_RESOURCE_HANDLE,
            .access = BufferAccess::StorageReadWrite,
            .binding = DescriptorBinding{0, 0, DescriptorUsageStage::ComputeShader}});

        graphicsTask->access({
            .handle = IMAGE_RESOURCE_HANDLE,
            .access = ImageAccess::SampledRead,
            .binding = DescriptorBinding{0, 0, DescriptorUsageStage::FragmentShader},
        });

        graphicsTask->access({.handle = BUFFER_RESOURCE_HANDLE,
            .access = BufferAccess::StorageReadWrite,
            .binding = DescriptorBinding{0, 1, DescriptorUsageStage::VertexShader}});

        graphicsTask->access({
            .handle = TARGET_RESOURCE_HANDLE,
            .access = ImageAccess::ColorAttachmentWrite,
            .binding = AttachmentLocation{0},
        });

        auto COMPUTE_TASK_HANDLE_1 = dependencyGraph->task(std::move(computeTask1));
        auto COMPUTE_TASK_HANDLE_2 = dependencyGraph->task(std::move(computeTask2));
        auto GRAPHICS_TASK_HANDLE = dependencyGraph->task(std::move(graphicsTask));

        dependencyGraph->dependency(COMPUTE_TASK_HANDLE_1, GRAPHICS_TASK_HANDLE, DependencyType::Debug);
        dependencyGraph->dependency(COMPUTE_TASK_HANDLE_2, GRAPHICS_TASK_HANDLE, DependencyType::Debug);

        dependencyGraph->present(GRAPHICS_TASK_HANDLE, TARGET_RESOURCE_HANDLE);

        auto compiler = std::make_unique<DependencyGraphCompiler>();
        auto compiledGraph = compiler->compile(*dependencyGraph);
    }());
}