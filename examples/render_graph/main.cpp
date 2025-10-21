#include <hammock/hammock.h>

using namespace Hammock;

std::string assetPath(const std::string& asset) {
    return "../../../data/" + asset;
}

std::string compiledShaderPath(const std::string& shader) {
    return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
}

// Here we create a simple logical render pass
class SimplePass final : public Rendergraph::ILogicalRenderPass {
   public:
    explicit SimplePass(const CreateInfo& createInfo)
        : ILogicalRenderPass(createInfo) {
    }

    // Called by render graph, Allocates resources
    void onCreate() override {
        Logger::info("Creating SimplePass");
    }

    // Called by render graph Frees allocated resources
    void onRelease() override {
    }

    // Called by render graph once to build the graph, Declare logical resources that are used
    void onDeclareResources() override {
    }

    // Called by render graph for every frame
    void onRecordCommands(VkCommandBuffer) override {
    }
};

int main() {
    VulkanInstance instance{};
    Hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    ResourceManager rm{device};
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
        .resourceManager = rm});

    // Add the pass
    /*renderGraph->addPass(std::make_unique<SimplePass>(Rendergraph::ILogicalRenderPass::CreateInfo{
        .commandQueueFamily = CommandQueueFamily::Graphics,
        .resourceManager = rm,
    }));*/

    renderGraph->addPass(
        Rendergraph::LogicalPassBuilder::create(CommandQueueFamily::Graphics, rm)
            .onCreate([] { Logger::info("Small pass created"); })
            .onRelease([] { Logger::info("Small pass released"); })
            .onDeclareResources([](Rendergraph::ILogicalRenderPass& pass) {
                // pass.read(...); pass.write(...);
            })
            .onRecordCommands([](VkCommandBuffer cmd) {
                // vkCmdBindPipeline(...);
            })
            .build());

    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();
    }
    device.waitIdle();
}
