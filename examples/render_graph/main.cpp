#include <iostream>
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
        Logger::info("Concrete pass created");
    }

    // Called by render graph Frees allocated resources
    void onRelease() override {
        Logger::info("Concrete pass released");
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

    renderGraph->useSwapChainImageResolver([&fm](){
        return Rendergraph::Graph::SwapChainImage{
            .image = fm.getSwapChain()->getImage(fm.getSwapChainImageIndex()),
            .imageView = fm.getSwapChain()->getImageView(fm.getSwapChainImageIndex()),
            .format = fm.getSwapChain()->getSwapChainImageFormat()
        };
    });

    renderGraph->addPass(
        Rendergraph::LogicalPassBuilder::create(HashName("LAMBDA_PASS"), CommandQueueFamily::Graphics, rm)
            .onCreate([] { Logger::info("Lambda pass created"); })
            .onRelease([] { Logger::info("Lambda pass released"); })
            .write(HashName(Rendergraph::Graph::SWAP_CHAIN_IMAGE_RESOURCE_NAME))
            .onRecordCommands([](VkCommandBuffer cmd) {
                Logger::info("Recording commands in lambda pass");
            })
            .build());

    if(const auto result = renderGraph->build(); !result) {
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
