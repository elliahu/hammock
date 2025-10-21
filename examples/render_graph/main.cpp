#include <hammock/hammock.h>

using namespace Hammock;

std::string assetPath(const std::string &asset) {
    return "../../../data/" + asset;
}

std::string compiledShaderPath(const std::string &shader) {
    return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
}

// Here we create a simple logical render pass
class SimplePass final : public Rendergraph::ILogicalRenderPass {
public:
    explicit SimplePass(const CreateInfo &createInfo)
        : ILogicalRenderPass(createInfo) {
    }

    void create() override {
        Logger::info("Creating SimplePass");
    }

    void release() override {

    }

    void declareResources() override {

    }

    void recordCommands(VkCommandBuffer) override {

    }
};

int main() {
    VulkanInstance instance{};
    Hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    ResourceManager rm{device};
    // TODO decouple context and window
    FrameManager fm{window, device, rm};
    std::unique_ptr<DescriptorPool> descriptorPool = DescriptorPool::Builder(device)
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
    auto renderGraph = std::make_unique<Rendergraph::Graph>(Rendergraph::Graph::CreateInfo{
        .resourceManager = rm
    });

    renderGraph->addPass(std::make_unique<SimplePass>(Rendergraph::ILogicalRenderPass::CreateInfo{
        .commandQueueFamily = CommandQueueFamily::Graphics,
        .resourceManager = rm,
    }));


    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();
    }
    device.waitIdle();
}
