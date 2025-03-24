#pragma once
#include "IRenderGroup.h"

/**
 * GeometryPass is responsible to draw a geometry into its color and depth buffer
 */
class GeometryPass final: public IRenderGroup {
public:
    // Type of the pass
    enum class Type {
        DepthOnly, // Fills only the depth buffer, set this for depth pre-pass
        ColorAndDepth // Fills both color and depth buffers
    };

    explicit GeometryPass(Device& device, ResourceManager& resourceManager): IRenderGroup(device, resourceManager){};

    void initialize(HmckVec2 resolution);

    void setType(const Type type) {this->type = type; }
    void setLightDirection(HmckVec3 lightDirection) {shaderData.lightDirection = HmckVec4{lightDirection,0.0f}; }
    void setLightColor(HmckVec3 lightColor) {shaderData.lightColor = HmckVec4{lightColor,0.0f}; }
    void setView(HmckMat4 view){this->view = view; }
    void setProjection(HmckMat4 projection){this->projection = projection; }

    Image * getColorTarget() const {return resourceManager.getResource<Image>(color);}
    Image * getDepthTarget() const {return resourceManager.getResource<Image>(depth);}

    void recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;

private:
    struct ShaderData {
        HmckMat4 modelViewProjection;
        HmckVec4 lightDirection;
        HmckVec4 lightColor;
    } shaderData;

    HmckMat4 view, projection;

    Type type = Type::DepthOnly;

    // Geometry buffers
    ResourceHandle vertexBuffer;
    ResourceHandle indexBuffer;

    // Actual geometry
    Geometry geometry;

    // Pipelines
    std::unique_ptr<GraphicsPipeline> depthOnlyPipeline;
    std::unique_ptr<GraphicsPipeline> colorAndDepthPipeline;

    // Targets
    ResourceHandle depth;
    ResourceHandle color;

    void prepareGeometry();
    void prepareTargets(uint32_t width, uint32_t height);
    void preparePipelines();

};
