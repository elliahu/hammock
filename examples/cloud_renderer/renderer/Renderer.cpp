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
        {.byteCode = Filesystem::readFile(TERRAIN_VERT_SHADER_PATH),},
        .fragmentShader
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
            .colorAttachmentFormats = {resourceManager.getResource<Image>(targets.terrainColor)->getFormat()},
            .depthAttachmentFormat = resourceManager.getResource<Image>(targets.terrainDepth)->getFormat(),
            // guaranteed to be supported on all hardware
        }
    });

    pipelines.compositionGraphics = GraphicsPipeline::create({
        .debugName = "composition-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPOSITION_VERT_SHADER_PATH),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPOSITION_FRAG_SHADER_PATH),},
        .descriptorSetLayouts = {descriptorLayouts.composition->getDescriptorSetLayout()},
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(targets.compositedColor)->getFormat()},
        }
    });

    pipelines.postprocessGraphics = GraphicsPipeline::create({
        .debugName = "postprocess-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(POSTPROC_VERT_SHADER_PATH),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(POSTPROC_FRAG_SHADER_PATH),},
        .descriptorSetLayouts = {descriptorLayouts.postprocess->getDescriptorSetLayout()},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PostProcessingData)}},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment (swap chain image)
            .colorAttachmentFormats = {frameManager.getSwapChain()->getSwapChainImageFormat()},
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

    // Composition
    VkDescriptorImageInfo terrainColorImageInfo = resourceManager.getResource<Image>(targets.terrainColor)->getDescriptorImageInfo(
        sampler->getSampler());
    VkDescriptorImageInfo terrainDepthImageInfo = resourceManager.getResource<Image>(targets.terrainDepth)->getDescriptorImageInfo(
        sampler->getSampler());
    DescriptorWriter(*descriptorLayouts.composition, *descriptorPool)
            .writeImage(0, &terrainColorImageInfo)
            .writeImage(1, &terrainDepthImageInfo)
            .build(descriptors.composition);

    // Postprocess
    VkDescriptorImageInfo compositedColorInfo = resourceManager.getResource<Image>(targets.compositedColor)->getDescriptorImageInfo(
        sampler->getSampler());
    DescriptorWriter(*descriptorLayouts.postprocess, *descriptorPool)
            .writeImage(0, &compositedColorInfo)
            .build(descriptors.postprocess);
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

    // Composition descriptor layout
    descriptorLayouts.composition = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain color image
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain depth image
            .build();

    // Postprocess descriptor layout
    descriptorLayouts.postprocess = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
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
        resourceManager.getResource<Buffer>(buffers.global[i])->map();
    }
}

void Renderer::createTargets() {
    // First, create the default sampler
    defaultSampler = resourceManager.createResource<Sampler>("default-sampler", SamplerDesc{});
    // Create all the images
    targets.compositedColor = resourceManager.createResource<Image>(
        "composited-color-image", ImageDesc{
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
    resourceManager.getResource<Image>(targets.compositedColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
    resourceManager.getResource<Image>(targets.terrainColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Terrain depth
    targets.terrainDepth = resourceManager.createResource<Image>(
        "terrain-depth", ImageDesc{
            .width = lWidth,
            .height = lHeight,
            .channels = 1,
            .format = VK_FORMAT_D32_SFLOAT, // Guaranteed support on all devices
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = {.depthStencil = {1.0f, 0}},
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(targets.terrainDepth)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

void Renderer::allocateCommandBuffers() {
    // Allocate graphics command buffers
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getGraphicsCommandPool();
    allocInfo.commandBufferCount = SwapChain::MAX_FRAMES_IN_FLIGHT;

    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.terrain.data()) == VK_SUCCESS,
           "Failed to allocate terrain command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.composition.data()) == VK_SUCCESS,
           "Failed to allocate post process command buffers!");

    // Allocate compute command buffers
    allocInfo.commandPool = device.getComputeCommandPool();
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.clouds.data()) == VK_SUCCESS,
           "Failed to allocate clouds command buffers!");
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.atmosphere.data()) == VK_SUCCESS,
           "Failed to allocate atmosphere command buffers!");
}

void Renderer::destroyCommandBuffers() {
    // Free graphics command buffers
    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.terrain.size()),
        commandBuffers.terrain.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.composition.size()),
        commandBuffers.composition.data());

    // Free compute command buffers
    vkFreeCommandBuffers(
        device.device(),
        device.getComputeCommandPool(),
        static_cast<uint32_t>(commandBuffers.clouds.size()),
        commandBuffers.clouds.data());

    vkFreeCommandBuffers(
        device.device(),
        device.getComputeCommandPool(),
        static_cast<uint32_t>(commandBuffers.atmosphere.size()),
        commandBuffers.atmosphere.data());
}

void Renderer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.cloudsReady[i]) == VK_SUCCESS,
               "Failed to create clouds ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.atmosphereReady[i]) == VK_SUCCESS,
               "Failed to create atmosphere ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.terrainReady[i]) == VK_SUCCESS,
               "Failed to create terrain ready semaphore!");
    }
}

void Renderer::destroySyncObjects() {
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device(), semaphores.cloudsReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.atmosphereReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.terrainReady[i], nullptr);
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
    allocateCommandBuffers();
    createSyncObjects();

    // Wait again
    device.waitIdle();

    // Delete staging buffers
    processDeletionQueue();
}

void Renderer::recordTerrainCommandBuffer() {
    // Get the current command buffer
    VkCommandBuffer commandBuffer = commandBuffers.terrain[frameManager.getFrameIndex()];
    // Begin the command buffer recording
    frameManager.beginCommandBuffer(commandBuffer);

    // Get render target pointers
    Image *terrainColorTarget = resourceManager.getResource<Image>(targets.terrainColor);
    Image *terrainDepthTarget = resourceManager.getResource<Image>(targets.terrainDepth);
    VkExtent3D renderingExtent = terrainColorTarget->getExtent();

    // Pipeline barriers
    if (terrainColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        // Record the transition to required layout
        terrainColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (terrainDepthTarget->getLayout() != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        // Record the transition to required layout
        terrainDepthTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    // Begin rendering
    VkRenderingAttachmentInfo colorTarget = terrainColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthTarget = terrainDepthTarget->getRenderingAttachmentInfo();
    depthTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    renderingInfo.renderArea = {0, 0, renderingExtent.width, renderingExtent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;
    renderingInfo.pDepthAttachment = &depthTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Viewport
    VkViewport viewport{0.0f, 0.0f, static_cast<float>(renderingExtent.width), static_cast<float>(renderingExtent.height), 0.0f, 1.0f};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Scissors
    VkRect2D scissor{0, 0, renderingExtent.width, renderingExtent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind global descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.terrainGraphics->pipelineLayout, 0, 1,
                            &descriptors.global[frameManager.getFrameIndex()], 0, nullptr);

    // Bind terrain pipeline
    pipelines.terrainGraphics->bind(commandBuffer);

    // Bind triangle vertex buffer (contains position and colors)
    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &resourceManager.getResource<Buffer>(buffers.vertexBuffer)->m_buffer, offsets);
    // Bind triangle index buffer
    vkCmdBindIndexBuffer(commandBuffer, resourceManager.getResource<Buffer>(buffers.indexBuffer)->m_buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    for (int i = 0; i < geometry.renderMeshes.size(); i++) {
        Geometry::MeshInstance &mesh = geometry.renderMeshes[i];
        data.terrainData.modelViewProjection = data.globalData.proj * data.globalData.view * mesh.transform;

        // Push block
        vkCmdPushConstants(commandBuffer, pipelines.terrainGraphics->pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(TerrainData), &data.terrainData);

        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, 0,
                         0);
    }

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);
}

void Renderer::recordCompositionCommandBuffer() {
    // Get the current command buffer
    VkCommandBuffer commandBuffer = commandBuffers.composition[frameManager.getFrameIndex()];
    // Begin the command buffer recording
    frameManager.beginCommandBuffer(commandBuffer);

    // Get attachment pointers
    Image *terrainColorTarget = resourceManager.getResource<Image>(targets.terrainColor);
    Image *terrainDepthTarget = resourceManager.getResource<Image>(targets.terrainDepth);
    Image *compositedColorTarget = resourceManager.getResource<Image>(targets.compositedColor);
    VkImage swapChainImage = frameManager.getSwapChain()->getImage(frameManager.getSwapChainImageIndex());
    VkImageView swapChainImageView = frameManager.getSwapChain()->getImageView(frameManager.getSwapChainImageIndex());
    VkExtent2D compositionRenderingExtent = {terrainColorTarget->getExtent().width, terrainDepthTarget->getExtent().height};

    // Transition attachments into required layouts
    if (terrainColorTarget->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (terrainDepthTarget->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainDepthTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (compositedColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        compositedColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    transitionImageLayout(commandBuffer, swapChainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          subresourceRange);

    // Begin rendering into intermediate composition image
    VkRenderingAttachmentInfo colorTarget = compositedColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo compositionRenderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    compositionRenderingInfo.renderArea = {0, 0, compositionRenderingExtent.width, compositionRenderingExtent.height};
    compositionRenderingInfo.layerCount = 1;
    compositionRenderingInfo.colorAttachmentCount = 1;
    compositionRenderingInfo.pColorAttachments = &colorTarget;

    // Composition
    vkCmdBeginRendering(commandBuffer, &compositionRenderingInfo);

    // Viewport
    VkViewport compositionViewport{
        0.0f, 0.0f, static_cast<float>(compositionRenderingExtent.width), static_cast<float>(compositionRenderingExtent.height), 0.0f, 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);

    // Scissors
    VkRect2D compositionScissors{0, 0, compositionRenderingExtent.width, compositionRenderingExtent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    // Bind composition descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.compositionGraphics->pipelineLayout, 0, 1,
                            &descriptors.composition, 0, nullptr);

    // Bind composition pipeline
    pipelines.compositionGraphics->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    // Transition intermediate image
    if (compositedColorTarget->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        compositedColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // We are now rendering into swap chain image
    VkExtent2D renderingExtent = frameManager.getSwapChain()->getSwapChainExtent();

    VkRenderingAttachmentInfo swapChainImageAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    swapChainImageAttachment.imageView = swapChainImageView;
    swapChainImageAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swapChainImageAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swapChainImageAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swapChainImageAttachment.clearValue.color = {0.0f, 0.0f, 0.2f, 0.0f};

    VkRenderingInfo postProcRenderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    postProcRenderingInfo.renderArea = {0, 0, renderingExtent.width, renderingExtent.height};
    postProcRenderingInfo.layerCount = 1;
    postProcRenderingInfo.colorAttachmentCount = 1;
    postProcRenderingInfo.pColorAttachments = &swapChainImageAttachment;

    // Begin rendering into swap chain image
    vkCmdBeginRendering(commandBuffer, &postProcRenderingInfo);

    // Viewport
    VkViewport postprocessViewport{
        0.0f, 0.0f, static_cast<float>(renderingExtent.width), static_cast<float>(renderingExtent.height), 0.0f, 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &postprocessViewport);

    // Scissors
    VkRect2D postprocessScissors{0, 0, renderingExtent.width, renderingExtent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &postprocessScissors);

    // Bind postprocess descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.postprocessGraphics->pipelineLayout, 0, 1,
                            &descriptors.postprocess, 0, nullptr);

    // Push postprocess data
    vkCmdPushConstants(commandBuffer, pipelines.postprocessGraphics->pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PostProcessingData), &data.postProcessingData);

    // Bind postprocessing pipeline
    pipelines.postprocessGraphics->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // End rendering
    vkCmdEndRendering(commandBuffer);

    // Records user interface into the same command buffer
    userInterface->recordUserInterface(commandBuffer);

    // Transition the swap chain image into present layout
    transitionImageLayout(commandBuffer, swapChainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          subresourceRange);
}

void Renderer::submitCommandBuffers() {
    // Get current frame index
    uint32_t frameIndex = frameManager.getFrameIndex();

    // Submit terrain command buffer
    frameManager.submitCommandBuffer<CommandQueueFamily::Graphics>(
        commandBuffers.terrain[frameIndex], {}, {semaphores.terrainReady[frameIndex]}, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    // Submit composition command buffer
    // This one is submitted for presentation
    frameManager.submitPresentCommandBuffer(commandBuffers.composition[frameIndex], semaphores.terrainReady[frameIndex]);
}

void Renderer::handleInput() {
    // Process rotation input using arrow keys.
    // Rotate left/right (yaw)
    if (window.isKeyDown(Surfer::KeyCode::ArrowLeft))
        camera.yaw += rotationSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowRight))
        camera.yaw -= rotationSpeed * deltaTime;

    // Rotate up/down (pitch)
    if (window.isKeyDown(Surfer::KeyCode::ArrowUp))
        camera.pitch += rotationSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowDown))
        camera.pitch -= rotationSpeed * deltaTime;

    // Clamp pitch to avoid excessive rotation
    const float pitchLimit = 1.55334f; // ~89 degrees in radians
    if (camera.pitch > pitchLimit)
        camera.pitch = pitchLimit;
    if (camera.pitch < -pitchLimit)
        camera.pitch = -pitchLimit;

    // Process translation input (WASD keys)
    if (window.isKeyDown(Surfer::KeyCode::KeyW))
        camera.position += camera.forwardDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyS))
        camera.position -= camera.forwardDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyA))
        camera.position += camera.rightDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyD))
        camera.position -= camera.rightDirection() * movementSpeed * deltaTime;

    if (window.isKeyDown(Surfer::KeyCode::Space))
        camera.position -= camera.upDirection() * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::LeftShift))
        camera.position += camera.upDirection() * movementSpeed * deltaTime;
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
    userInterface = std::make_unique<::UserInterface>(
        device,
        frameManager,
        descriptorPool->descriptorPool,
        window,
        camera,
        data.globalData,
        deltaTime,
        frameTimes,
        FRAMETIME_BUFFER_SIZE,
        frameTimeFrameIndex);
    init();
}

Renderer::~Renderer() {
    destroyCommandBuffers();
    destroySyncObjects();
}

void Renderer::update() {
    // Update camera
    camera.aspect = frameManager.getAspectRatio();

    // update global data
    data.globalData.fov = camera.fov;
    data.globalData.proj = camera.getProjection();
    data.globalData.view = camera.getView();
    data.globalData.invProj = HmckInvGeneral(data.globalData.proj);
    data.globalData.invView = HmckInvGeneral(data.globalData.view);
    data.globalData.cameraPosition = HmckVec4{camera.position, 0.0f};
    data.globalData.resX = window.getExtent().width;
    data.globalData.resY = window.getExtent().height;
    data.globalData.time = elapsedTime;
    resourceManager.getResource<Buffer>(buffers.global[frameManager.getFrameIndex()])->writeToBuffer(&data.globalData);
}


void Renderer::render() {
    // Get the current time
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Initialize the rendering loop
    while (!window.shouldClose()) {
        // Poll for events
        window.pollEvents();

        // Handle input
        handleInput();

        // Update the timing
        auto newTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        elapsedTime += deltaTime;
        currentTime = newTime;

        // Update frame time tracking
        frameTimes[frameTimeFrameIndex] = deltaTime * 1000.0f;
        frameTimeFrameIndex = (frameTimeFrameIndex + 1) % FRAMETIME_BUFFER_SIZE;

        // Record and submit frame
        if (frameManager.beginFrame()) {
            // Update the data for the frame
            update();

            // Records command buffers
            recordTerrainCommandBuffer();
            recordCompositionCommandBuffer();

            // Submit command buffers
            submitCommandBuffers();

            // Submit frame
            frameManager.endFrame();
        }
    }

    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
