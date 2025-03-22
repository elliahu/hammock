#pragma once
#include <hammock/hammock.h>
#include "Types.h"
#include "Camera.h"
#include "UserInterface.h"

// working directory
#define CWD(path) "../../../examples/cloud_renderer/" path

// assets
#define ASSET_PATH(asset) CWD("assets/" asset)
#define TERRAIN_GEOMETRY_PATH ASSET_PATH("terrain.glb")
#define HIGH_FREQ_NOISE_PATH ASSET_PATH("detail")
#define LOW_FREQ_NOISE_PATH ASSET_PATH("base")
#define WEATHER_MAP_PATH ASSET_PATH("weather/stratocumulus.png")

// shaders
#define COMPILED_SHADER_PATH(shader) CWD("spv/" shader ".spv")
#define CLOUDS_COMP_SHADER_PATH COMPILED_SHADER_PATH("clouds.comp")
#define GOD_RAYS_COMP_SHADER_PATH COMPILED_SHADER_PATH("occlusion.comp")
#define TERRAIN_VERT_SHADER_PATH COMPILED_SHADER_PATH("terrain.vert")
#define TERRAIN_FRAG_SHADER_PATH COMPILED_SHADER_PATH("terrain.frag")
#define COMPOSITION_VERT_SHADER_PATH COMPILED_SHADER_PATH("compose.vert")
#define COMPOSITION_FRAG_SHADER_PATH COMPILED_SHADER_PATH("compose.frag")
#define POSTPROC_VERT_SHADER_PATH COMPILED_SHADER_PATH("postprocess.vert")
#define POSTPROC_FRAG_SHADER_PATH COMPILED_SHADER_PATH("postprocess.frag")

// resolutions
#define CLOUDS_OCCLUSION_MASK_RELATIVE_SIZE 1.0f
#define CLOUDS_OCCLUSION_MASK_SIZE_X(w) (w * CLOUDS_OCCLUSION_MASK_RELATIVE_SIZE)
#define CLOUDS_OCCLUSION_MASK_SIZE_Y(y) (y * CLOUDS_OCCLUSION_MASK_RELATIVE_SIZE)

// Work groups
#define CLOUDS_WORK_GROUP_SIZE_X 16
#define CLOUDS_WORK_GROUP_SIZE_Y 16
#define GOD_RAYS_WORK_GROUP_SIZE_X 16
#define GOD_RAYS_WORK_GROUP_SIZE_Y 16
#define CLOUDS_GROUPS_X(w) ((w + CLOUDS_WORK_GROUP_SIZE_X - 1) / CLOUDS_WORK_GROUP_SIZE_X)
#define CLOUDS_GROUPS_Y(h) ((h + CLOUDS_WORK_GROUP_SIZE_Y - 1) / CLOUDS_WORK_GROUP_SIZE_Y)
#define GOD_RAYS_GROUPS_X(w) ((w + GOD_RAYS_WORK_GROUP_SIZE_X - 1) / GOD_RAYS_WORK_GROUP_SIZE_X)
#define GOD_RAYS_GROUPS_Y(h) ((h + GOD_RAYS_WORK_GROUP_SIZE_Y - 1) / GOD_RAYS_WORK_GROUP_SIZE_Y)

using namespace hammock;

/**
 * This is the main renderer class
 * It uses hammock engine under the hood, which is my custom Vulkan abstraction layer
 */
class Renderer final{
    // Vulkan instance
    VulkanInstance instance{};
    // Window class, uses hammock's window class which stands on top of VulkanSurfer lib
    hammock::Window window;
    // Physical device (GPU)
    Device device;
    // Resource manages is responsible for resource alloc/release on device
    ResourceManager resourceManager;
    // Frame manager handles queue submission and contains swap chain abstraction
    FrameManager frameManager;
    // Descriptor pool is used to allocate descriptor sets and layouts
    std::unique_ptr<DescriptorPool> descriptorPool;
    // User interface
    std::unique_ptr<::UserInterface> userInterface;

    // Deletion queue is used to queue resources that should be deleted
    std::queue<ResourceHandle> deletionQueue;

    // Geometry object representing scenes triangle geometry (in this case terrain)
    Geometry geometry;

    // Launch dimensions
    uint32_t lWidth, lHeight;

    // Benchmarking
    float deltaTime{0.f}, elapsedTime{0.f};
    static constexpr int FRAMETIME_BUFFER_SIZE{512}; // Number of frames to track
    float frameTimes[FRAMETIME_BUFFER_SIZE] = {0.0f};
    int frameTimeFrameIndex{0};
    bool progressTime{true};

    // Data passed to the gpu
    struct {
        // Cloud properties data
        CloudsProperties cloudsProperties;
        // Blur properties data
        BlurProperties blurProperties;
        // Shared date
        GlobalData globalData;
        // Terrain data
        TerrainData terrainData;
        // Postprocessing data
        PostProcessingData postProcessingData;
    } data;

    // Perspective camera
    Camera camera{
        HmckVec3{0.f, 3.0f, 0.f},
        static_cast<float>(lWidth) /  static_cast<float>(lHeight),
        HmckToRad(HmckAngleDeg(45.f)), 0.01f, 1000.f};

    // Movement
    const float movementSpeed = 1.0f; // Units per frame
    const float rotationSpeed = 1.0f; // Radians per frame

    // Resources

    // Default sampler
    ResourceHandle defaultSampler;

    struct {
        // Global buffers, one for each frame in flight
        std::array<ResourceHandle, SwapChain::MAX_FRAMES_IN_FLIGHT> global;
        // Vertex buffer
        ResourceHandle vertexBuffer;
        // Index buffer
        ResourceHandle indexBuffer;
    } buffers;

    struct {
        // Low freq noise is used to create base shape of the cloud
        ResourceHandle lowFrequencyNoise;
        // High freq noise is used to create detail shape on top of the base shape of the cloud
        ResourceHandle highFrequencyNoise;
        // Weather map describes weather state in the scene
        ResourceHandle weatherMap;
    } assets;

    struct {
        // Composited image
        ResourceHandle compositedColor;

        // Clouds storage image
        ResourceHandle cloudsColor;
        // Clouds density mask image
        ResourceHandle cloudsOcclusionColor;
        // Clouds god rays color image
        ResourceHandle godRaysColor;

        // Terrain image
        ResourceHandle terrainColor;
        // Terrain depth
        ResourceHandle terrainDepth;
    } targets;

    // Here it is important to minimize the number of descriptor sets per pass as some devices may only support as little as 4
    // There is a one global descriptor set accessible from both queues and then each pass only uses up to one other set
    // If possible no additional sets are used and all the data passed to the render pass is passed using push block which is fast
    struct {
        // Shared descriptor set
        std::array<VkDescriptorSet, SwapChain::MAX_FRAMES_IN_FLIGHT> global;
        // Descriptor set for clouds contains only static resources (render targets and textures) that does not require to be per-frame
        VkDescriptorSet clouds;
        // Occlusion mask blur descriptor
        VkDescriptorSet godRays;
        // Composition descriptor
        VkDescriptorSet composition;
        // Post process descriptor
        VkDescriptorSet postprocess;
    } descriptors;

    struct {
        // Shared descriptor set layout
        std::unique_ptr<DescriptorSetLayout> global;
        // clouds descriptor set layout
        std::unique_ptr<DescriptorSetLayout> clouds;
        // god rays descriptor set layout
        std::unique_ptr<DescriptorSetLayout> godRays;
        // composition descriptor set layout
        std::unique_ptr<DescriptorSetLayout> composition;
        // post process descriptor set layout
        std::unique_ptr<DescriptorSetLayout> postprocess;
    } descriptorLayouts;

    // Pipelines
    struct {
        // Clouds compute pipeline
        std::unique_ptr<ComputePipeline> cloudsCompute;

        // God rays compute pipeline
        std::unique_ptr<ComputePipeline> godRaysCompute;

        // Terrain graphics pipeline
        std::unique_ptr<GraphicsPipeline> terrainGraphics;

        // Composition graphics pipeline
        std::unique_ptr<GraphicsPipeline> compositionGraphics;

        // Post process pipeline
        std::unique_ptr<GraphicsPipeline> postprocessGraphics;
    } pipelines;

    // Command buffers
    // There is one command buffer per group per frame,
    // so that the cpu can record commands for next frame while gpu processes commands from current frame
    struct {
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> clouds; // Clouds and blur
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> atmosphere;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> terrain;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> composition; // Composition and postprocess
    } commandBuffers;

    // Semaphores signal that the command buffer is finished so that the command buffer waiting for its result can start
    struct {
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> cloudsReady;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> atmosphereReady;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> terrainReady;
    } semaphores;


    /**
     * Queues resource for deletion
     * @param resource Resource that will be deleted
     * @return ResourceHandle
     */
    ResourceHandle queueForDeletion(ResourceHandle resource) {
        deletionQueue.push(resource);
        return resource;
    }

    /**
     * Deletes all items in the queue
     */
    void processDeletionQueue();

    /**
     * Build all graphics and compute pipelines
     */
    void buildPipelines();

    /**
     * Builds all the descriptor sets
     */
    void buildDescriptorSets();

    /**
    * Builds all the descriptor set layouts
    */
    void buildDescriptorSetLayouts();

    /**
     * Creates all the uniform buffers
     */
    void createBuffers();

    /**
     * Creates all images
     */
    void createTargets();

    /**
     * Loads all assets
     */
    void loadAssets();

    /**
     * Allocates command buffer one per frame per queue
     */
    void allocateCommandBuffers();

    /**
     * Destroys all command buffers
     */
    void destroyCommandBuffers();

    /**
     * Creates synchronization primitives
     */
    void createSyncObjects();

    /**
     * Destroys synchronization primitives
     */
    void destroySyncObjects();

    /**
     * Initializes the renderer
     */
    void init();

    /**
     * Records terrain commands into its command buffer
     */
    void recordTerrainCommandBuffer();

    /**
     * Records composition commands into its command buffer
     */
    void recordCompositionCommandBuffer();

    /**
     * Records clouds and blur dispatches into its command buffer
     */
    void recordCloudsCommandBuffer();

    /**
     * Submits recorded command buffer to their corresponding queues
     */
    void submitCommandBuffers();

    /**
     * All input related code is in here
     */
    void handleInput();

    /**
     * Called every frame before the draw
     * this is used to update the buffers etc.
     */
    void update();




public:
    // Constructor
    Renderer(const int32_t width, const int32_t height);
    // Destructor
    ~Renderer();

    /**
     * Initiates the rendering loop
     */
    void render();

};