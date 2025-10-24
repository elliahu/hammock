#include <hammock/hammock.h>

#include <cstddef>

#include "hammock/core/GraphicsPipeline.h"
#include "hammock/utils/Filesystem.h"

using namespace Hammock;

auto getCompiledShaderPath(const std::string& name) -> std::string {
    return std::string(HAMMOCK_BUILD_DIR) + "/spv/" + name;
}

int main() {
    VulkanInstance instance{};
    Hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    ResourceManager rm{device};
    FrameManager fm{window, device, rm};
    auto descriptorPool = DescriptorPool::Builder(device)
                              .setMaxSets(20000)
                              .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000)
                              .build();

    // Create the rendergraph instance
    auto renderGraph = std::make_unique<Rendergraph::Graph>(Rendergraph::Graph::CreateInfo{.resourceManager = rm});

    renderGraph->useSwapChainImageResolver([&fm]() {
        return Rendergraph::Graph::SwapChainImage{.image = fm.getSwapChain()->getImage(fm.getSwapChainImageIndex()),
            .imageView = fm.getSwapChain()->getImageView(fm.getSwapChainImageIndex()),
            .format = fm.getSwapChain()->getSwapChainImageFormat()};
    });


    // Build the render graph
    if (const auto result = renderGraph->build(); !result) {
        Logger::error("Rendergraph build failed: %s", result.error().c_str());
        exit(EXIT_FAILURE);
    }

    renderGraph->execute();

    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();
    }
    device.waitIdle();
}
