#pragma once
#include "IRenderGroup.h"
#include "Types.h"


class PostProcessingPass final : public IRenderGroup {
public:
    struct PostProcData {
        HmckVec4 colorTint{1.0f, 1.0f, 1.0f, 0.0f};
        float exposure = 2.3f; // Default: 0.0, Range: -5.0 to 5.0
        float gamma = 2.2f; // Default: 2.2, Range: 0.5 to 3.0
        int tonemapOperator = 3; // 0: Linear, 1: Reinhard, 2: ACES, 3: Uncharted 2
        float contrast = 1.0; // Default: 1.0, Range: 0.5 to 2.0
        float brightness = 0.0; // Default: 0.0, Range: -1.0 to 1.0
        float saturation = 1.0; // Default: 1.0, Range: 0.0 to 2.0
        float vignetteStrength = 2.f; // Default 2.0, Range: 0.0 to 3.0
        float vignetteSoftness = 1.0f; // Default: 0.5, Range: 0.0 to 2.0
        float temperature = 0.0f; // Default: 0.0, Range: -1.0 (cool) to 1.0 (warm)
        float grainAmount = 0.0f; // Default: 0.0, Range: 0.0 to 0.1
        float time = 0.0f;
    } data;

    PostProcessingPass(Device &device, ResourceManager &resourceManager)
        : IRenderGroup(device, resourceManager) {
    }

    void initialize();

    void setIinput(Image * image) {input = image;}
    void setSwapChainImage(VkImage image) { swapChainImage = image; }
    void setSwapChainImageView(VkImageView imageView) { swapChainImageView = imageView; }
    void setSwapChainImageFormat(VkFormat format) { swapChainImageFormat = format; }
    void setSwapChainRenderingExtent(VkExtent2D extent) {renderingExtent = extent; }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:

    // Target
    VkImage swapChainImage;
    VkImageView swapChainImageView;
    ResourceHandle sampler;

    // Input
    Image * input;
    VkFormat swapChainImageFormat;
    VkExtent2D renderingExtent;


    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> pipeline;

    void prepareSampler() {
        sampler = resourceManager.createResource<Sampler>("composition-sampler", SamplerDesc{});
    }
    void prepareDescriptors();
    void preparePipelines();
};
