#include <memory>

#include "app/application.hpp"
#include "app/application_types.hpp"
#include "dependency_graph/dependency_graph.hpp"
#include "dependency_graph/dependency_graph_compiler.hpp"
#include "dependency_graph/gpu_task.hpp"
#include "image.hpp"

using namespace hammock;

int main() {
    auto app = std::make_unique<app::Application>(app::ExecutionMode::Engine);

   /* auto dg = std::make_unique<graph::DependencyGraph>();

    auto targetHandle = dg->image({
        .format = core::ImageFormat::R8G8B8A8Uint,
        .usage = core::ImageUsage::ColorAttachment,
        .width = 1920u,
        .height = 1080u,
    });

    auto task = std::make_unique<graph::GraphicsTask>(
        "../../spv/user_interface.vert.spv", "../../spv/user_interface.frag.spv");

    task->access(graph::LogicalResourceAccess{
        targetHandle, graph::ImageAccess::ColorAttachmentWrite, graph::AttachmentLocation{0}});

    auto taskHandle = dg->task(std::move(task));
    dg->present(taskHandle, targetHandle);

    auto comp = std::make_unique<graph::DependencyGraphCompiler>(app->getVulkanContext());
    auto cpg = comp->compile(*dg);*/

    app->launch();

    return EXIT_SUCCESS;
}
