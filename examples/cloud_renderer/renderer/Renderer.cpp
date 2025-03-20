#include "Renderer.h"

void Renderer::processDeletionQueue() {
    while (!deletionQueue.empty()) {
        auto item = deletionQueue.front();
        resourceManager.releaseResource(item.getUid());
        deletionQueue.pop();
    }
}

void Renderer::buildPipelines() {
    // Clouds compute pipeline
    // pipelines.cloudsCompute = ComputePipeline::create({
    //     .debugName = "compute-pipeline",
    //     .device = device,
    //     .computeShader{.byteCode = Filesystem::readFile(CLOUDS_COMP_SHADER_PATH),},
    //     .descriptorSetLayouts = {
    //         descriptorLayouts.global->getDescriptorSetLayout(),
    //         descriptorLayouts.clouds->getDescriptorSetLayout(),
    //     },
    //     .pushConstantRanges{{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CloudsProperties)}}
    // });

    // Terrain graphics pipeline
    pipelines.terrainGraphics = GraphicsPipeline::create({
        .debugName = "terrain-pipeline",
        .device = device,
        .vertexShader
        // Fullscreen vertex shader
        {.byteCode = Filesystem::readFile(TERRAIN_VERT_SHADER_PATH),},
        .fragmentShader
        // Fragment shader samples storage texture and writes it to swapchain image
        {.byteCode = Filesystem::readFile(TERRAIN_FRAG_SHADER_PATH),},
        .descriptorSetLayouts = {descriptorLayouts.global->getDescriptorSetLayout()},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TerrainData)}},
        .graphicsState{
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {VK_FORMAT_R8G8B8A8_UNORM},
            .depthAttachmentFormat = VK_FORMAT_D16_UNORM, // guaranteed to be supported on all hardware
        }
    });
}

void Renderer::buildDescriptorSets() {
    // Default sampler
    Sampler *sampler = resourceManager.getResource<Sampler>(defaultSampler);

    // global descriptor set
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = resourceManager.getResource<Buffer>(buffers.global[i])->descriptorInfo();
        DescriptorWriter(*descriptorLayouts.global, *descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .build(descriptors.global[i]);
    }

    // Clouds
    VkDescriptorImageInfo cloudsImageInfo = resourceManager.getResource<Image>(targets.cloudsColor)->getDescriptorImageInfo(
        sampler->getSampler());
    VkDescriptorImageInfo cloudsMaskImageInfo = resourceManager.getResource<Image>(targets.cloudsMaskColor)->getDescriptorImageInfo(
        sampler->getSampler());
    VkDescriptorImageInfo lowFreqNoiseInfo = resourceManager.getResource<Image>(assets.lowFrequencyNoise)->getDescriptorImageInfo(
        sampler->getSampler());
    VkDescriptorImageInfo highFreqNoiseInfo = resourceManager.getResource<Image>(assets.highFrequencyNoise)->getDescriptorImageInfo(
        sampler->getSampler());
    VkDescriptorImageInfo weatherMapInfo = resourceManager.getResource<Image>(assets.weatherMap)->getDescriptorImageInfo(
        sampler->getSampler());
    DescriptorWriter(*descriptorLayouts.clouds, *descriptorPool)
            .writeImage(0, &cloudsImageInfo)
            .writeImage(1, &cloudsMaskImageInfo)
            .writeImage(2, &lowFreqNoiseInfo)
            .writeImage(3, &highFreqNoiseInfo)
            .writeImage(4, &weatherMapInfo)
            .build(descriptors.clouds);
}

void Renderer::buildDescriptorSetLayouts() {
    // Global descriptor layout
    descriptorLayouts.global = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // global buffer
            .build();

    // Clouds descriptor layout
    descriptorLayouts.clouds = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Clouds image
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Clouds mask image
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // low freq noise
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // high freq noise
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // weather map
            .build();
}

void Renderer::createBuffers() {
    // Create vertex buffer
    ASSERT(!geometry.vertices.empty(), "No vertices loaded! Cannot create vertex buffer");
    buffers.vertexBuffer = resourceManager.createResource<Buffer>(
        "vertex-buffer", BufferDesc{
            .instanceSize = sizeof(Vertex),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .currentQueueFamily = CommandQueueFamily::Ignored,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );

    // Create vertex staging buffer
    ResourceHandle vertexStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "vertex-staging-buffer", BufferDesc{
            .instanceSize = sizeof(Vertex),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write vertex data to the buffer
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->map();
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->writeToBuffer(geometry.vertices.data());

    // Copy data from staging buffer to actual vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * geometry.vertices.size();
    resourceManager.getResource<Buffer>(buffers.vertexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(vertexStagingBuffer)->getBuffer(), vertexBufferSize);
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->unmap();

    // Create index buffer
    ASSERT(!geometry.indices.empty(), "No indices loaded! Cannot create index buffer");
    buffers.indexBuffer = resourceManager.createResource<Buffer>(
        "index-buffer", BufferDesc{
            .instanceSize = sizeof(uint32_t),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .currentQueueFamily = CommandQueueFamily::Ignored,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );

    // Create index staging buffer
    ResourceHandle indexStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "index-staging-buffer", BufferDesc{
            .instanceSize = sizeof(uint32_t),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write index data into the staging buffer
    resourceManager.getResource<Buffer>(indexStagingBuffer)->map();
    resourceManager.getResource<Buffer>(indexStagingBuffer)->writeToBuffer(geometry.indices.data());

    // Copy the data from staging buffer into actual index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * geometry.indices.size();
    resourceManager.getResource<Buffer>(buffers.indexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(indexStagingBuffer)->getBuffer(), indexBufferSize);
    resourceManager.getResource<Buffer>(indexStagingBuffer)->unmap();


    // Create global uniform buffers
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        buffers.global[i] = resourceManager.createResource<Buffer>(
            "global-buffer-" + std::to_string(i), BufferDesc{
                .instanceSize = sizeof(GlobalData),
                .instanceCount = 1,
                .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
                // Global buffer is enabled to be access from both queues at the same time without the need for bariers
                .sharingMode = VK_SHARING_MODE_CONCURRENT,
            }
        );
    }
}

void Renderer::createTargets() {
    // First, create the default sampler
    defaultSampler = resourceManager.createResource<Sampler>("default-sampler", SamplerDesc{});
    // Create all the images
    // Clouds image
    targets.cloudsColor = resourceManager.createResource<Image>(
        "clouds-image", ImageDesc{
            .width = lWidth,
            .height = lHeight,
            .channels = 4,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(targets.cloudsColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);

    // Clouds mask image
    targets.cloudsMaskColor = resourceManager.createResource<Image>(
        "clouds-mask-image", ImageDesc{
            .width = static_cast<uint32_t>(lWidth * CLOUD_MASK_FRAC),
            .height = static_cast<uint32_t>(lHeight * CLOUD_MASK_FRAC),
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(targets.cloudsMaskColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);

    // Terrain image
    targets.terrainColor = resourceManager.createResource<Image>(
        "terrain-image", ImageDesc{
            .width = lWidth,
            .height = lHeight,
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .clearValue = {.color = {0.f, 0.f, 0.f, 0.f}},
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(targets.terrainColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Terrain depth
    targets.terrainDepth = resourceManager.createResource<Image>(
        "terrain-depth", ImageDesc{
            .width = lWidth,
            .height = lHeight,
            .channels = 4,
            .format = VK_FORMAT_D16_UNORM, // Guaranteed support on all devices
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = {.depthStencil = {1.0f, 1}},
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(targets.terrainDepth)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Renderer::loadAssets() {
    // Load the terrain
    Loader(geometry, device, resourceManager).loadglTF(TERRAIN_GEOMETRY_PATH);

    int w, h, c, d;
    // Low frequency noise
    {
        // Read the data from disk
        AutoDelete lowFreqNoiseData(readVolume(Filesystem::ls(LOW_FREQ_NOISE_PATH), w, h, c, d,
                                               Filesystem::ImageFormat::R16G16B16A16_SFLOAT), [](const void *p) {
            delete[] static_cast<const float16_t *>(p);
        });
        // Create staging buffer
        ResourceHandle lowFreqNoiseStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "base-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(float16_t),
                .instanceCount = static_cast<uint32_t>(w * h * d * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));
        // Write data to the buffer
        resourceManager.getResource<Buffer>(lowFreqNoiseStagingBuffer)->map();
        resourceManager.getResource<Buffer>(lowFreqNoiseStagingBuffer)->writeToBuffer(lowFreqNoiseData.get());

        // Create the actual image resource
        assets.lowFrequencyNoise = resourceManager.createResource<Image>(
            "base-noise",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .depth = static_cast<uint32_t>(d),
                .mips = getNumberOfMipLevels(static_cast<uint32_t>(w), static_cast<uint32_t>(h)),
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_3D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
            }
        );

        // Copy data from buffer to image
        resourceManager.getResource<Image>(assets.lowFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(assets.lowFrequencyNoise)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(lowFreqNoiseStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(assets.lowFrequencyNoise)->generateMips();
        resourceManager.getResource<Image>(assets.lowFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    // High frequency noise
    {
        // Read the data from disk
        AutoDelete highFrequencyNoiseData(readVolume(Filesystem::ls(HIGH_FREQ_NOISE_PATH), w, h, c, d,
                                                     Filesystem::ImageFormat::R16G16B16A16_SFLOAT), [](const void *p) {
            delete[] static_cast<const float16_t *>(p);
        });
        // Create staging buffer
        ResourceHandle highFrequencyNoiseStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "detail-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(float16_t),
                .instanceCount = static_cast<uint32_t>(w * h * d * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));

        // Write data to the buffer
        resourceManager.getResource<Buffer>(highFrequencyNoiseStagingBuffer)->map();
        resourceManager.getResource<Buffer>(highFrequencyNoiseStagingBuffer)->writeToBuffer(highFrequencyNoiseData.get());

        // Create the actual image resource
        assets.highFrequencyNoise = resourceManager.createResource<Image>(
            "detail-noise",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .depth = static_cast<uint32_t>(d),
                .mips = getNumberOfMipLevels(static_cast<uint32_t>(w), static_cast<uint32_t>(h)),
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                // We only use RGB channels but most devices do not support 3D RGB textures so we use RGBA
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_3D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
            }
        );

        // Copy data from buffer to image
        resourceManager.getResource<Image>(assets.highFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(assets.highFrequencyNoise)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(highFrequencyNoiseStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(assets.highFrequencyNoise)->generateMips();
        resourceManager.getResource<Image>(assets.highFrequencyNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    // Weather map
    {
        AutoDelete weatherMapData(readImage(WEATHER_MAP_PATH, w, h, c,
                                            Filesystem::ImageFormat::R8G8B8A8_UNORM), [](const void *p) {
            delete[] static_cast<const uchar8_t *>(p);
        });

        // Create host visible staging buffer on device
        ResourceHandle weatherMapStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
            "weather-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(uchar8_t),
                .instanceCount = static_cast<uint32_t>(w * h * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        ));

        // Write the data into the staging buffer
        resourceManager.getResource<Buffer>(weatherMapStagingBuffer)->map();
        resourceManager.getResource<Buffer>(weatherMapStagingBuffer)->writeToBuffer(weatherMapData.get());

        // Create the image resource
        assets.weatherMap = resourceManager.createResource<Image>(
            "weather-map",
            ImageDesc{
                .width = static_cast<uint32_t>(w),
                .height = static_cast<uint32_t>(h),
                .channels = static_cast<uint32_t>(c),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_2D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            }
        );

        // Copy the data from buffer into the image
        resourceManager.getResource<Image>(assets.weatherMap)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        resourceManager.getResource<Image>(assets.weatherMap)->queueCopyFromBuffer(
            resourceManager.getResource<Buffer>(weatherMapStagingBuffer)->getBuffer());
        resourceManager.getResource<Image>(assets.weatherMap)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void Renderer::init() {
    loadAssets();

    // Loading assets submitted command buffers that need to finish before we proceed
    device.waitIdle();

    // Create the rest
    createBuffers();
    createTargets();
    buildDescriptorSetLayouts();
    buildDescriptorSets();
    buildPipelines();

    // Wait again
    device.waitIdle();

    // Delete staging buffers
    processDeletionQueue();
}

Renderer::Renderer(const int32_t width, const int32_t height)
    : window{instance, "Vulkan atmospheric renderer", static_cast<int>(width), static_cast<int>(height)},
      device{instance, window.getSurface()},
      resourceManager{device},
      frameManager{window, device},
      lWidth{static_cast<uint32_t>(width)}, lHeight{static_cast<uint32_t>(height)} {
    // Initialize the descriptor pool object from which descriptors will be allocated
    descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .build();
    init();
}

void Renderer::update() {
}


void Renderer::render() {
    // Get the current time
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Initialize the rendering loop
    while (!window.shouldClose()) {

        // Poll for events
        window.pollEvents();

        // Update the timing
        auto newTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        elapsedTime += deltaTime;
        currentTime = newTime;

        // Update frame time tracking
        frameTimes[frameTimeFrameIndex] = deltaTime * 1000.0f;
        frameTimeFrameIndex = (frameTimeFrameIndex + 1) % FRAMETIME_BUFFER_SIZE;

        // Update the data for the frame
        update();


    }

    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
