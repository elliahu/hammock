#include "IScene.h"

#define EARTH_RADIUS 6371000.0

class SkyScene final : public IScene {
    // Compute work group and local size computation
    static constexpr uint32_t WORKGROUP_SIZE_X = 32;
    static constexpr uint32_t WORKGROUP_SIZE_Y = 32;
    static constexpr uint32_t groupsX = (1920 + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
    static constexpr uint32_t groupsY = (1080 + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

    // Uniform data updated every frame
    // Lives in a GPU memory that is visible to CPU

    struct CameraUbo {
        HmckMat4 invView;
        HmckMat4 invProj;
        HmckMat4 invViewProj;
        HmckVec4 cameraPosition;
        float32_t resX;
        float32_t resY;
        float32_t fov;
    } cameraUbo;


    struct TimeUbo {
        float32_t time = 0.0f;
    } timeUbo;

    bool progressTime = false;

    struct PostProcPushConsts {
        float whitePoint = 1.0f;
        float exposure = 2.5;
        float gamma = 2.2;
        float time = 0.0f;
    } postProcPushConsts;

    struct SunAndSkyUbo {
        HmckVec4 cloudColorTop{0.99f, 0.876f, 0.876f, 1.0f};
        HmckVec4 cloudColorBottom{0.382f, 0.411f, 0.470f, 1.0f};
        HmckVec4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
        HmckVec4 lightDirection{0.0f, 1.0f, 0.f, 0.0f};
        HmckVec4 skyColorBottom{0.462f, 0.654f, 0.956f, 1.0f};
        HmckVec4 skyColorTop{0.376f,0.443f, 0.843f, 1.0};
    } sunAndSkyUbo;

    struct ComputePushConsts {
        float coverageOverride = 0.3f;
        float baseCoverageMultiplier = 2.0f;
        float cloudSpeed = 450.f;
        float crispiness = 2.0f; // .4
        float curliness = 10.0f;
        float absorption = 0.0035f; //0.0035
        float densityFactor =  0.02f; //  0.02;
        int enablePowder = 1; // 0
        float fogFactor = 0.00006f;
        float earthRadius = 70000.0f; // 35000, 70000
        float cloudsInnerRadius = 6000.0f; // 5000, 6000
        float cloudsOuterRadius = 27000.0f; // 17000, 27000
        float phaseG = 0.3f;
        float ambientStrength = 2.0f;
    } computePushConsts;


    // Compute pass resources
    struct {
        // Compute pipeline to draw the clouds in parallel patches
        std::unique_ptr<ComputePipeline> cloudPipeline;

        // Base cloud noise
        ResourceHandle baseNoise;

        // Detail cloud noise
        ResourceHandle detailNoise;

        // Curl cloud noise
        ResourceHandle curlNoise;

        // Cloud map
        ResourceHandle cloudMap;

        // Other resources are managed on-the-fly by the rendergraph
    } compute;

    // Composition pass resources
    struct {
        // Graphics pipeline that is used to compose the final image and write it to the swapchain image
        std::unique_ptr<GraphicsPipeline> pipeline;
        // Other resources are managed on-the-fly by the rendergraph
    } composition;

    // Post-processing pass
    struct {
        std::unique_ptr<GraphicsPipeline> toneMapPipeline;
    } postProc;


    // This is used to measure frame time
    float32_t deltaTime = 0.0f;
    float32_t totalElapsedTime = 0.0f;
    uint32_t frameCount = 0;

    // camera movement
    float32_t yaw{0.f}, pitch{0.f};
    HmckVec3 cameraPosition{0.f, 0.f, 0.f};
    float32_t fov = 45.f;


    // Benchmarking
    // Constants
    static constexpr int FRAMETIME_BUFFER_SIZE = 512; // Number of frames to track
    // Variables
    float frameTimes[FRAMETIME_BUFFER_SIZE] = {0.0f};
    int frameTimeFrameIndex = 0;

public:
    SkyScene(const std::string &name, const uint32_t width, const uint32_t height)
        : IScene(name, width, height) {
        init();
        // Some of the calls in inti may submit a command buffer so we need to wait for it to finish before we start rendering
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
