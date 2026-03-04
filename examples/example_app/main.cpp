#include <memory>
#include <print>

#include "app/application.hpp"
#include "app/application_types.hpp"
#include "dependency_graph/dependency_graph.hpp"
#include "dependency_graph/dependency_graph_compiler.hpp"
#include "dependency_graph/dependency_graph_executor.hpp"
#include "dependency_graph/gpu_task.hpp"
#include "device.hpp"
#include "image.hpp"

using namespace hammock;

int main() {
    auto app = std::make_unique<app::Application>(app::ExecutionMode::Engine);

    auto dg = std::make_unique<graph::DependencyGraph>();

    auto targetHandle = dg->image({
        .format = core::ImageFormat::R8G8B8A8Uint,
        .usage = core::ImageUsage::ColorAttachment,
        .width = 1920u,
        .height = 1080u,
    });

    auto task = graph::GpuTask(core::CommandQueueFamily::Graphics);

    task.access({targetHandle, graph::ImageAccess::ColorAttachmentWrite});

    task.compile([](graph::GpuTaskCompileContext& ctx) { std::println("Compiling"); });

    task.exec([](graph::GpuTaskExecContext& ctx) { std::println("Executing"); });

    auto taskHandle = dg->task(std::move(task));
    dg->root(taskHandle);

    auto comp = std::make_unique<graph::DependencyGraphCompiler>(app->getVulkanContext());
    auto cdg = comp->compile(*dg);

    auto exec = std::make_unique<graph::DependencyGraphExecutor>();
    exec->execute(cdg);

    app->launch();

    return EXIT_SUCCESS;
}
