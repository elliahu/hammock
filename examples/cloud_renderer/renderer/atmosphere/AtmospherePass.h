#pragma once
#include "Transmittance.h"
#include "MultipleScattering.h"ů
#include "SkyView.h"
#include "../IRenderGroup.h"
#include "../Types.h"


class AtmospherePass final : public IRenderGroup {
public:
    AtmospherePass(Device &device, ResourceManager &resourceManager)
        : IRenderGroup(device, resourceManager),
          transmittance(device, resourceManager),
          multipleScattering(device, resourceManager),
          skyView(device, resourceManager) {
    }

    void initialize();

    void setEye(HmckVec3 eye) {
        atmosphere.eye = HmckVec4{eye, 0.0f};
    }

    void setSunDirection(HmckVec3 sunDirection) {
        atmosphere.sunDirection = HmckVec4{sunDirection, 0.0f};
    }

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

    AtmosphereUniformBufferData atmosphere;

private:
    // Luts
    Transmittance transmittance;
    MultipleScattering multipleScattering;
    SkyView skyView;

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
