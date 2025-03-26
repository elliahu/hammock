#pragma once
#include "ILookUpTable.h"

#define AERIAL_PERSPECTIVE_LUT_SIZE_X 32
#define AERIAL_PERSPECTIVE_LUT_SIZE_Y 32
#define AERIAL_PERSPECTIVE_LUT_SIZE_Z 32


class AerialPerspective final : public ILookUpTable {
public:
    AerialPerspective(Device &device, ResourceManager &resourceManager)
        : ILookUpTable(device, resourceManager) {
    }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    void initialize(VkDescriptorSetLayout descriptorSetLayout) override;

    void setMultipleScattering(Image *image) { multipleScattering = image; }
    void setTransmittance(Image *image) { transmittance = image; }
    void setShadowMap(Image *image) { shadowMap = image; }
    void setShadowViewProjection(HmckMat4 mat) { data.shadowViewProjection = mat; }
    void setCameraFrustum(HmckVec4 a,HmckVec4 b,HmckVec4 c,HmckVec4 d) {
        data.frustumA = a;
        data.frustumB = b;
        data.frustumC = c;
        data.frustumD = d;
    }

protected:
    struct AerialPerspectiveData {
        HmckMat4 shadowViewProjection;
        HmckVec4 frustumA;
        HmckVec4 frustumB;
        HmckVec4 frustumC;
        HmckVec4 frustumD;
    } data;


    Image *multipleScattering;
    Image *transmittance;
    Image *shadowMap;


    void prepareLut() override;

    void prepareDescriptors() override;

    void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) override;
};
