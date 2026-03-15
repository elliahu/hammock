#include <memory>
#include <print>

#include "app/application.hpp"
#include "app/application_types.hpp"
#include "render_graph/render_graph_compiler.hpp"

using namespace hammock;

int main() {
    auto app = std::make_unique<app::Application>(app::ExecutionMode::Engine);

    auto rg = std::make_unique<graph::RenderGraph>();

    auto targetHandle = rg->image({
        .format = core::ImageFormat::R8G8B8A8Uint,
        .usage = core::ImageUsage::ColorAttachment,
        .width = 1920u,
        .height = 1080u,
    });

    auto pass = graph::RenderPass(core::CommandQueueFamily::Graphics);

    pass.build([&targetHandle](graph::RenderPassBuilder& builder) {
        builder.access({targetHandle, graph::ImageAccess::ColorAttachmentWrite});
    });

    pass.compile([](graph::RenderPassComplContext& ctx) { std::println("Compiling"); });

    pass.exec([](graph::RenderPassExecContext& ctx) { std::println("Executing"); });

    auto taskHandle = rg->pass(std::move(pass));
    rg->root(taskHandle);

    auto comp = std::make_unique<graph::RenderGraphCompiler>();
    auto cdg = comp->compile(*rg);

    app->launch();

    return EXIT_SUCCESS;
}
