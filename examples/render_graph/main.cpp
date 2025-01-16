#include <hammock/hammock.h>

using namespace hammock;


int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    DeviceStorage storage{device};
    // TODO decouple context and window
    RenderContext renderContext{window, device};

    std::unique_ptr<RenderGraph> graph = std::make_unique<RenderGraph>(device, renderContext);

    ResourceNode swap;
    swap.name = "swap-image";
    swap.type = ResourceNode::Type::SwapChain;
    swap.isExternal = true; // this image is managed by swapchain
    swap.refs = renderContext.getSwapChain()->getSwapChainImageRefs();
    swap.desc = renderContext.getSwapChain()->getSwapChainImageDesc();
    graph->addResource(swap);

    // This image has no resource ref as it will be created and manged by RenderGraph
    ResourceNode depth;
    depth.name = "depth-image";
    depth.type = ResourceNode::Type::Image;
    depth.desc = ImageDesc{
        .size = {1.0f, 1.0f}, // SwapChain relative by default
        .format = renderContext.getSwapChain()->getSwapChainDepthFormat(),
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    };
    graph->addResource(depth);

    RenderPassNode shadowPass;
    shadowPass.name = "shadow-pass";
    shadowPass.type = RenderPassNode::Type::Graphics;
    shadowPass.extent = renderContext.getSwapChain()->getSwapChainExtent();
    shadowPass.outputs.push_back({
        "depth-image", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE
    });
    shadowPass.executeFunc = [&](RenderPassContext context) {
        std::cout << "Shadow pass executed" << std::endl;
    };
    graph->addPass(shadowPass);

    RenderPassNode compositionPass;
    compositionPass.name = "composition-pass";
    compositionPass.type = RenderPassNode::Type::Graphics;
    compositionPass.extent = renderContext.getSwapChain()->getSwapChainExtent();
    compositionPass.inputs.push_back({
        "depth-image", VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD
    });
    compositionPass.outputs.push_back({
        "swap-image", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE
    });
    compositionPass.executeFunc = [&](RenderPassContext context) {
        std::cout << "Composition pass executed" << std::endl;
    };
    graph->addPresentPass(compositionPass);
    graph->compile();

    while (!window.shouldClose()) {
        window.pollEvents();

        graph->execute();
    }
    device.waitIdle();
}
