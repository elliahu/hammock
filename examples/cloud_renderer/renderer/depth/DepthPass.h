#pragma once
#include "../IRenderGroup.h"
#include "../Types.h"


/**
 * This pass generates two depth images
 * first one from the camera - to be used for depth culling in the clouds pass
 * second one from the sun - to be used to generate areal-perspective LUT
 */
class DepthPass final : public IRenderGroup {
public:
    DepthPass(Device &device, ResourceManager &resourceManager, Geometry &geometry)
        : IRenderGroup(device, resourceManager), geometry(geometry) {
    }

    void initialize(HmckVec2 resolution);

    void setVertexBuffer(Buffer *buffer) { vertexBuffer = buffer; }
    void setIndexBuffer(Buffer *buffer) { indexBuffer = buffer; }
    void setCameraView(HmckMat4 mat) { cameraView = mat; }
    void setCameraProjection(HmckMat4 mat) { cameraProjection = mat; }
    void setSunView(HmckMat4 mat) { sunView = mat; }
    void setSunProjection(HmckMat4 mat) { sunProjection = mat; }

    Image * getCameraDepth() const {return resourceManager.getResource<Image>(cameraDepth);}
    Image * getSunDepth() const {return resourceManager.getResource<Image>(sunDepth);}

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:
    HmckMat4 cameraView, cameraProjection;
    HmckMat4 sunView, sunProjection;

    // Vertex and index buffers
    Buffer *vertexBuffer;
    Buffer *indexBuffer;
    Geometry &geometry;

    // Pipeline
    std::unique_ptr<GraphicsPipeline> pipeline;

    // Targets
    ResourceHandle cameraDepth;
    ResourceHandle sunDepth;

    void prepareTargets(uint32_t width, uint32_t height);

    void preparePipelines();
};
