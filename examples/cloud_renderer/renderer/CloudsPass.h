#pragma once
#include "IRenderGroup.h"

#define CLOUDS_WORK_GROUP_SIZE_X 16
#define CLOUDS_WORK_GROUP_SIZE_Y 16
#ifndef REPROJECTION
#define CLOUDS_GROUPS_X(w) ((w + CLOUDS_WORK_GROUP_SIZE_X - 1) / CLOUDS_WORK_GROUP_SIZE_X)
#define CLOUDS_GROUPS_Y(h) ((h + CLOUDS_WORK_GROUP_SIZE_Y - 1) / CLOUDS_WORK_GROUP_SIZE_Y)
#else
#define CLOUDS_GROUPS_X(w) (ceil(w/4/8))
#define CLOUDS_GROUPS_Y(h) (ceil(h/4/8))
#endif

class CloudsPass final : public IRenderGroup {
public:
    CloudsPass(Device &device, ResourceManager &resourceManager)
        : IRenderGroup(device, resourceManager) {
    }

    // This is passed as buffer
    struct UniformBufferData {
        HmckMat4 invView;
        HmckMat4 invProj;
        HmckMat4 view;
        HmckMat4 proj;
        HmckVec4 cameraPosition;
        HmckVec4 lightColor{1.0f, 1.0f, 1.0f, 5.0f}; // W is strength
        HmckVec4 lightDirection{0.0f, 1.0f, 0.f, 0.0f};
        HmckVec4 skyColorZenith{59.0 / 255.0, 110.0 / 255.0, 219.0 / 255.0};
        HmckVec4 skyColorHorizon{169.0 / 255.0, 175.0 / 255.0, 188.0 / 255.0};
        HmckVec4 windDirection;
        float resX;
        float resY;
        float fov;
        float znear;
        float zfar;
        float time = 0.0f;
        float timeOfDay = 0.45f;
        int frameIndexMod16;
    } uniform;

    // This is passed as push data
    struct PushConstantData {
        float anvilBias = 0.0f;
        float globalDensity = 0.3f;
        float globalCoverage = 0.0;
        float baseMultiplier = 0.8f;
        float detailMultiplier = 0.75;
        float cloudSpeed = 0.f;
        float baseScale = 50.f;
        float detailScale = 50.0f;
        float curliness = 2.0f;
        float absorption = 0.0074f;
        float eccentricity = 0.22f;
        float intensity = 10.0f;
        float spread = 1.0f;
        float ambientStrength = 0.05;
        int DEBUG_epicView = 0;
        int DEBUG_cheapSampleDistance = 80000;
        int DEBUG_maxSamples = 96;
        int DEBUG_maxLightSamples = 4;
        int DEBUG_expensiveSampling = 0;
        int DEBUG_earlyTermination = 0;
        int DEBUG_lateTermination = 0;
        int DEBUG_longStepMulti = 2;
    } properties;


    void initialize(HmckVec2 resolution);


    void setView(const HmckMat4 &view) {
        uniform.view = view;
        uniform.invView = HmckInvGeneral(view);
    }
    void setProjection(const HmckMat4 &projection) {
        uniform.proj = projection;
        uniform.invProj = HmckInvGeneral(projection);
    }

    Image * getColorTarget() const {return resourceManager.getResource<Image>(color);}

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:
    // The uniform buffers
    std::array<ResourceHandle, SwapChain::MAX_FRAMES_IN_FLIGHT> uniformBuffers;

    // Descriptors
    std::unique_ptr<DescriptorSetLayout> layout;
    std::array<VkDescriptorSet, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptors;

    // Compute pipeline
    std::unique_ptr<ComputePipeline> pipeline;

    // Targets
    ResourceHandle color;

    // Resources
    ResourceHandle lowFrequencyNoise;
    ResourceHandle highFrequencyNoise;
    ResourceHandle weatherMap;
    ResourceHandle sampler;

    void prepareBuffers();
    void prepareResources();
    void prepareTargets(uint32_t width, uint32_t height);
    void prepareDescriptors();
    void preparePipelines();
};
