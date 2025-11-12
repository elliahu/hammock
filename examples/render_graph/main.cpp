#include <hammock/hammock.h>

#include <memory>

using namespace Hammock;
using namespace Rendergraph;

auto getCompiledShaderPath(const std::string& name) -> std::string {
    return std::string(HAMMOCK_BUILD_DIR) + "/spv/" + name;
}

int main() {
    VulkanInstance instance{};
    Hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};

    ResourceManager::initialize(device);

    FrameManager::initialize(window, device);

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
    DescriptorPool& dp = DescriptorPool::getInstance();

    Scoped deleter([] {
        ResourceManager::dispose();
        FrameManager::dispose();
        DescriptorPool::dispose();
    });

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
            .colorAttachmentFormats = {FrameManager::getInstance().getSwapChain()->getSwapChainImageFormat()},
        }});

    // Create the rendergraph instance
    auto renderGraph = std::make_unique<Graph>(device);

    auto OUT_COLOR = renderGraph->useSwapChainImages([] {
        auto& fm = FrameManager::getInstance();
        return Graph::SwapChainImage{
            .image = fm.getSwapChain()->getImage(fm.getSwapChainImageIndex()),
            .imageView = fm.getSwapChain()->getImageView(fm.getSwapChainImageIndex()),
            .format = fm.getSwapChain()->getSwapChainImageFormat(),
        };
    });

    renderGraph->addPass(GraphicsPass("COLOR1")
            .write({OUT_COLOR})
            .bind({})
            .vertex({Filesystem::readFile(getCompiledShaderPath("fullscreen.vert.spv"))})
            .fragment({Filesystem::readFile(getCompiledShaderPath("fullscreen.frag.spv"))})
            .execute([](ExecutionContext& context) {

            }));

    /* // Build the render graph
    if (const auto result = renderGraph->build(); !result) {
        Logger::error("Rendergraph build failed: %s", result.error().c_str());
        exit(EXIT_FAILURE);
    }

    renderGraph->execute(); */

    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();
    }
    device.waitIdle();
}
