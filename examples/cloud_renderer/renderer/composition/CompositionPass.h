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
    void setSkyView(Image * image) {skyViewLUT = image; }
    void setInvView(HmckMat4 mat) {data.inverseView = mat; }
    void setInvProjection(HmckMat4 mat) {data.inverseProjection = mat; }
    void setCameraFrustum(HmckVec4 a,HmckVec4 b,HmckVec4 c,HmckVec4 d) {
        data.frustumA = a;
        data.frustumB = b;
        data.frustumC = c;
        data.frustumD = d;
    }

    Image* getColorTarget() { return resourceManager.getResource<Image>(compositedImage); }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:

    struct CompositionData {
        HmckMat4 inverseView;
        HmckMat4 inverseProjection;
        HmckVec4 frustumA;
        HmckVec4 frustumB;
        HmckVec4 frustumC;
        HmckVec4 frustumD;
        float resX;
        float resY;
    } data;


    // Target
    ResourceHandle compositedImage; // Final composited image that is passed to the post procsessing pass
    ResourceHandle skyColor; // Sky color image up-sampled from skyViewLUT
    ResourceHandle sampler;
    ResourceHandle buffer;

    // Inputs
    Image *terrainColor;
    Image *terrainDepth;
    Image *cloudsColor;
    Image *skyViewLUT;
    Image *aerialPerspectiveLUT;
    Image *sunShadow;
    ResourceHandle blueNoise;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> compositionLayout;
    VkDescriptorSet compositionDescriptor;
    std::unique_ptr<DescriptorSetLayout> skyLayout;
    VkDescriptorSet skyDescriptor;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> compositionPipeline;
    std::unique_ptr<GraphicsPipeline> skyPipeline;

    void prepareBlueNoise();
    void prepareBuffer();
    void prepareTargets(uint32_t width, uint32_t height);
    void prepareDescriptors();
    void preparePipelines();
};
