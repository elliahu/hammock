#include <hammock/hammock.h>

#include <cstddef>
#include <memory>

#include "hammock/core/GraphicsPipeline.h"
#include "hammock/rendergraph/ExecutionContext.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/resources/Descriptors.h"
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

    DescriptorPool::initialize(device,
        1000,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000},
        });
    auto& descPool = DescriptorPool::getInstance();

        auto descriptorSetLayout =
            DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

    auto pipeline = GraphicsPipeline::create({.debugName = "present-pipeline",
        .device = device,
        .vertexShader{
            .byteCode = Filesystem::readFile(getCompiledShaderPath("fullscreen.vert.spv")),
        },
        .fragmentShader{
            .byteCode = Filesystem::readFile(getCompiledShaderPath("fullscreen.frag.spv")),
        },
        .descriptorSetLayouts = {descriptorSetLayout->getDescriptorSetLayout()},
        .pushConstantRanges{},
        .graphicsState{.cullMode = VK_CULL_MODE_NONE, .vertexBufferBindings{}},
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {fm.getSwapChain()->getSwapChainImageFormat()},
        }});

    // Create the rendergraph instance
    auto renderGraph = std::make_unique<Rendergraph::Graph>(Rendergraph::Graph::CreateInfo{
        .device = device,
        .resourceManager = rm,
    });

    auto scColor = renderGraph->useSwapChainImages([&fm]() {
        return Rendergraph::Graph::SwapChainImage{
            .image = fm.getSwapChain()->getImage(fm.getSwapChainImageIndex()),
            .imageView = fm.getSwapChain()->getImageView(fm.getSwapChainImageIndex()),
            .format = fm.getSwapChain()->getSwapChainImageFormat(),
        };
    });

    renderGraph->addPass(Rendergraph::PassBuilder<Rendergraph::GraphicsPass>("COLOR1")
            .write({scColor})
            .bindings({{scColor, 0}})
            .pipeline(std::move(pipeline))
            .execute([](Rendergraph::ExecutionContext context) { Logger::debug("Executing COLOR1"); })
            .build());

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
