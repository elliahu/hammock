#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"

class CompositionPass final : public IRenderGroup {
public:
    CompositionPass(Device &device, ResourceManager &resourceManager)
        : IRenderGroup(device, resourceManager) {
    }

    void initialize(HmckVec2 resolution);

    void setTerrainColor(Image *image) { terrainColor = image; }
    void setTerrainDepth(Image *image) { terrainDepth = image; }
    void setCloudsColor(Image *image) { cloudsColor = image; }

    Image* getColorTarget() { return resourceManager.getResource<Image>(color); }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:
    // Target
    ResourceHandle color;
    ResourceHandle sampler;

    // Inputs
    Image *terrainColor;
    Image *terrainDepth;
    Image *cloudsColor;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> pipeline;

    void prepareTargets(uint32_t width, uint32_t height);
    void prepareDescriptors();
    void preparePipelines();
};
