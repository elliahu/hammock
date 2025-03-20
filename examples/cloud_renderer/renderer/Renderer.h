#pragma once
#include <hammock/hammock.h>
#include "Types.h"

// working directory
#define CWD(path) "../../../examples/cloud_renderer/" path

// assets
#define ASSET_PATH(asset) CWD("assets/" asset)
#define TERRAIN_GEOMETRY_PATH ASSET_PATH("terrain.glb")
#define HIGH_FREQ_NOISE_PATH ASSET_PATH("base")
#define LOW_FREQ_NOISE_PATH ASSET_PATH("detail")
#define WEATHER_MAP_PATH ASSET_PATH("weather/stratocumulus.png")

// shaders
#define COMPILED_SHADER_PATH(shader) CWD("spv/" shader ".spv")
#define CLOUDS_COMP_SHADER_PATH COMPILED_SHADER_PATH("clouds.comp")

// resolutions
#define CLOUD_MASK_FRAC 0.25f

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

    // Deletion queue is used to queue resources that should be deleted
    std::queue<ResourceHandle> deletionQueue;

    // Geometry object representing scenes triangle geometry (in this case terrain)
    Geometry geometry;

    // Launch dimensions
    uint32_t lWidth, lHeight;

    // Data passed to the gpu
    struct {
        // Cloud properties data
        CloudsProperties cloudsProperties;
        // Shared date
        GlobalData globalData;
    } data;

    // Resources
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
        // Default sampler
        ResourceHandle defaultSampler;
        // Clouds storage image
        ResourceHandle cloudsImage;
        // Clouds density mask image
        ResourceHandle cloudsMaskImage;
    } images;

    struct {
        // Shared descriptor set
        std::array<VkDescriptorSet, SwapChain::MAX_FRAMES_IN_FLIGHT> global;
        VkDescriptorSet clouds;
    } descriptors;

    struct {
        // Shared descriptor set layout
        std::unique_ptr<DescriptorSetLayout> global;
        // clouds descriptor set layout
        std::unique_ptr<DescriptorSetLayout> clouds;
    } descriptorLayouts;

    // Pipelines
    struct {
        // Clouds compute pipeline
        std::unique_ptr<ComputePipeline> cloudsCompute;
    } pipelines;

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
    void createImages();

    /**
     * Loads all assets
     */
    void loadAssets();

    /**
     * Initializes the renderer
     */
    void init();

    /**
     * Called every frame before the draw
     */
    void update();



public:
    // Constructor
    Renderer(const int32_t width, const int32_t height);

    void render();

};