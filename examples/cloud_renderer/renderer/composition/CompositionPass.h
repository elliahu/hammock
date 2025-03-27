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
    void setSkyView(Image * image) {skyView = image; }
    void setInvView(HmckMat4 mat) {data.inverseView = mat; }
    void setInvProjection(HmckMat4 mat) {data.inverseProjection = mat; }

    Image* getColorTarget() { return resourceManager.getResource<Image>(color); }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:

    struct CompositionData {
        HmckMat4 inverseView;
        HmckMat4 inverseProjection;
    } data;


    // Target
    ResourceHandle color;
    ResourceHandle sampler;

    // Inputs
    Image *terrainColor;
    Image *terrainDepth;
    Image *cloudsColor;
    Image *skyView;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> pipeline;

    void prepareTargets(uint32_t width, uint32_t height);
    void prepareDescriptors();
    void preparePipelines();
};
