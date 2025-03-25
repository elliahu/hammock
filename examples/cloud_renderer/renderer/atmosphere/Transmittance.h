#pragma once
#include "ILookUpTable.h"

/**
 * Responsible for creating transmittance LUT
 */
class Transmittance : public ILookUpTable {
public:
    Transmittance(Device &device, ResourceManager &resourceManager)
        : ILookUpTable(device, resourceManager) {
    }

    void initialize(VkDescriptorSetLayout descriptorSetLayout) override;

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

protected:
    void prepareDescriptors() override;

    void prepareLut() override;

    void preparePipeline(VkDescriptorSetLayout descriptorSetLayout) override;

};
