#include "IScene.h"

#define EARTH_RADIUS 6371000.0

class SkyScene final : public IScene {
    // Compute work group and local size computation
    static constexpr uint32_t WORKGROUP_SIZE_X = 16;
    static constexpr uint32_t WORKGROUP_SIZE_Y = 16;
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
        float32_t timeOfDay = 0.0f;
    } timeUbo;

    bool progressTime = false;

    struct SunAndSkyUbo {
        HmckVec4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
        HmckVec4 lightDirection{0.0f, 1.0f, 0.f, 0.0f};
        HmckVec4 skyColorZenith{59.0/255.0, 110.0/255.0, 219.0/255.0};
        HmckVec4 skyColorHorizon{169.0/255.0, 175.0/255.0, 188.0/255.0};
        HmckVec4 windDirection;
    } sunAndSkyUbo;

    struct ComputePushConsts {
        float anvilBias = 0.0f;
        float globalDensity = 0.3f;
        float globalCoverage = 0.0;
        float baseMultiplier = 0.8f;
        float detailMultiplier = 0.75;
        float cloudSpeed = 1000.f;
        float baseScale = 75.f;
        float detailScale = 100.0f;
        float curliness = 2.0f;
        float absorption = 0.0014f;
        float scattering = 0.2f;
        float eccentricity = 0.6f;
        float silverIntensity = 1.73f;
        float silverSpread = 0.887f;
        float ambientStrength = 0.85f;
        int DEBUG_cheapSampleDistance = 100000;
        int DEBUG_maxSamples = 128;
        int DEBUG_maxLightSamples = 6;
        int DEBUG_expensiveSampling = 0;
        int DEBUG_earlyTermination = 0;
        int DEBUG_lateTermination = 0;
    } computePushConsts, backUpComputePushConsts;

    float32_t timeOfDay = 0.5f;
    float32_t windDirection = 0.0f;

    struct PostProcessUBO {
        HmckVec4 colorTint{1.0f, 1.0f, 1.0f,0.0f};        // Default: vec3(1.0, 1.0, 1.0)

        // Tonemapping parameters
        float exposure = 2.3f;        // Default: 0.0, Range: -5.0 to 5.0
        float gamma = 2.2f;           // Default: 2.2, Range: 0.5 to 3.0
        int tonemapOperator = 3;   // 0: Linear, 1: Reinhard, 2: ACES, 3: Uncharted 2

        // Color grading parameters
        float contrast = 1.0;        // Default: 1.0, Range: 0.5 to 2.0
        float brightness = 0.0;      // Default: 0.0, Range: -1.0 to 1.0
        float saturation = 1.0;      // Default: 1.0, Range: 0.0 to 2.0

        // Vignette
        float vignetteStrength = 2.f; // Default 2.0, Range: 0.0 to 3.0
        float vignetteSoftness = 1.0f; // Default: 0.5, Range: 0.0 to 2.0

        // Color temperature
        float temperature = 0.0f;     // Default: 0.0, Range: -1.0 (cool) to 1.0 (warm)

        // Film grain
        float grainAmount = 0.0f;     // Default: 0.0, Range: 0.0 to 0.1

        // Cloud blending mode
        int cloudBlendMode = 0.0;  // Default: 0.0 (Normal)

        float time = 0.0f;
    } postProcUbo, backUpPostProcUbo;



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

    struct {
        std::unique_ptr<GraphicsPipeline> pipeline;

        ResourceHandle skyDome;
    } sky;

    // Composition pass resources
    struct {
        // Graphics pipeline that is used to compose the final image and write it to the swapchain image
        std::unique_ptr<GraphicsPipeline> pipeline;
        // Other resources are managed on-the-fly by the rendergraph
    } composition;


    // This is used to measure frame time
    float32_t deltaTime = 0.0f;
    float32_t totalElapsedTime = 0.0f;
    uint32_t frameCount = 0;

    // camera movement
    float32_t yaw{-1.2187f}, pitch{0.4235f};
    HmckVec3 cameraPosition{0.f, 3.f, 0.f};
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
