#pragma once
#include "Transmittance.h"
#include "MultipleScattering.h"ů
#include "../IRenderGroup.h"
#include "../Types.h"


class AtmospherePass final : public IRenderGroup {
public:
    AtmospherePass(Device &device, ResourceManager &resourceManager)
        : IRenderGroup(device, resourceManager), transmittance(device, resourceManager), multipleScattering(device, resourceManager) {
    }

    void initialize();

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    AtmosphereUniformBufferData atmosphere;

private:
    // Luts
    Transmittance transmittance;
    MultipleScattering multipleScattering;

    // Buffers
    // There is one common buffer for all luts bound once at the start of the pass
    ResourceHandle atmosphereBuffer;
    // Then each dispatch uses its own buffer for its custom data

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    VkDescriptorSet descriptor;

    void prepareBuffers();

    void prepareDescriptors();
};
