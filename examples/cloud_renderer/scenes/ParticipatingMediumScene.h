#include "IScene.h"

class ParticipatingMediumScene final : public IScene
{
    // Signed distance field used to describe the shape of our volume
    ResourceHandle sdf;
    // 3D noise to sample the density from
    ResourceHandle densityNoise;
    // Curl noise used to offset the sampling position to create wave-like effect
    ResourceHandle curlNoise;
    // Blue noise for offset
    ResourceHandle blueNoise;

    struct UniformBuffer {
        HmckMat4 view;
        HmckMat4 proj;
        HmckVec4 eye;
        HmckVec4 lightPosition {5.0f, .5f, 0.0f};
        HmckVec4 lightColor {1.0f, 1.0f, 1.0f, 1.0f};
        float32_t resX;
        float32_t resY;
        float32_t elapsedTime;
    } ubo;

    struct PushConstants {
        HmckVec4 scattering{0.25,0.25,0.2};
        HmckVec4 absorption{0.02, 0.01, 0.005};
        float mieG = 0.34;
        float densityMultiplier = 1.0;
        int jitter = 1;
        float jitterStrength = 5.0f;
    } pushConstants;


    // A master graphics pipeline
    std::unique_ptr<GraphicsPipeline> pipeline;

    // This is used to measure frame time
    float32_t deltaTime = 0.0f;
    bool progressTime = true;

    float32_t radius{2.0f}, azimuth{0.0f}, elevation{0.0f};

public:
    ParticipatingMediumScene(const std::string &name, const uint32_t width, const uint32_t height)
        : IScene(name, width, height)
    {
        init();
        device.waitIdle();
    }

     // Initializes all resources
     void init() override;

     // Builds the rendergraph
     void buildRenderGraph();
 
     // Builds pipelines for each pass
     void buildPipelines();
 
     // This gets called every frame and updates the data that is then passed to the uniform buffer
     void update();
 
     // Render loop
     void render() override;
};