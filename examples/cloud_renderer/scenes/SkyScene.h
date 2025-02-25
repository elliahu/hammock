#include "IScene.h"

class SkyScene final : public IScene {
    // Compute work group and local size computation
    static constexpr uint32_t WORKGROUP_SIZE_X = 32;
    static constexpr uint32_t WORKGROUP_SIZE_Y = 32;
    static constexpr uint32_t groupsX = (1920 + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
    static constexpr uint32_t groupsY = (1080 + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

    // Uniform data updated every frame
    // Lives in a GPU memory that is visible to CPU

    struct CameraUbo {
        HmckMat4 view;
        HmckMat4 proj;
        HmckVec4 eye;
        HmckVec2 tanFovBy2;
    } cameraUbo, oldCameraUbo;

    bool isOldCameraEmpty = true;

    struct TimeUbo {
        HmckVec4 haltonSeq1;
        HmckVec4 haltonSeq2;
        HmckVec4 haltonSeq3;
        HmckVec4 haltonSeq4;
        HmckVec2 time;
        uint32_t frameCountMod16;
    } timeUbo;

    struct PostProcPushConsts {
        float whitePoint = 1.0f;
        float exposure = 2.5;
        float gamma = 2.2;
        float time;
    } postProcPushConsts;

    struct ComputePushConsts {
        float cloudCoverageOverride = 0.6f;
        float baseDensityFactor = 0.38f;
        float samplingFrequency = 8.0f;
        float highFreqDensityMult = .5f;
        float lightBrightness = 5.0f;
        int debugBackgroundSky = 0;
        int debugCloudDensity = 0;
        int debugTextureCurlNoise = 0;
        int debugTextureBaseNoise = 0;
        int debugTextureDetailNoise = 0;
        int debugHeightGradient = 0;
        int debugTTest = 0;
        int debugPhaseTest = 0;
        int debugBeerTest = 0;
    } computePushConsts;

    // TODO SunAndSkyUbo

    // Compute pass resources
    struct {
        std::unique_ptr<ComputePipeline> reprojectionPipeline;

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


    // Benchmarking
    // Constants
    static constexpr int FRAMETIME_BUFFER_SIZE = 512; // Number of frames to track
    // Variables
    float frameTimes[FRAMETIME_BUFFER_SIZE] = { 0.0f };
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
