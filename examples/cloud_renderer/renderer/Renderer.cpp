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
    pipelines.cloudsCompute = ComputePipeline::create({
        .debugName = "compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(CLOUDS_COMP_SHADER_PATH),},
        .descriptorSetLayouts = {
            descriptorLayouts.global->getDescriptorSetLayout(),
            descriptorLayouts.clouds->getDescriptorSetLayout(),
        },
        .pushConstantRanges{{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CloudsProperties)}}
    });
}

void Renderer::buildDescriptorSets() {
    // Default sampler
    Sampler *sampler = resourceManager.getResource<Sampler>(images.defaultSampler);

    // global descriptor set
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = resourceManager.getResource<Buffer>(buffers.global[i])->descriptorInfo();
        DescriptorWriter(*descriptorLayouts.global, *descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .build(descriptors.global[i]);
    }

    // Clouds
    VkDescriptorImageInfo cloudsImageInfo = resourceManager.getResource<Image>(images.cloudsImage)->getDescriptorImageInfo(
        sampler->getSampler());
    VkDescriptorImageInfo cloudsMaskImageInfo = resourceManager.getResource<Image>(images.cloudsMaskImage)->getDescriptorImageInfo(
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

    buffers.vertexBuffer = resourceManager.createResource<Buffer>(
           "vertex-buffer", BufferDesc{
               .instanceSize = sizeof(Vertex),
               .instanceCount = static_cast<uint32_t>(10),
               .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
           }
       );
    // Create vertex staging buffer
    /*ResourceHandle vertexStagingBuffer = resourceManager.createResource<Buffer>(
        "vertex-staging-buffer", BufferDesc{
            .instanceSize = sizeof(geometry.vertices[0]),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });*/

    // Copy geometry data to the staging buffer
    //resourceManager.getResource<Buffer>(vertexStagingBuffer)->map();
   // resourceManager.getResource<Buffer>(vertexStagingBuffer)->writeToBuffer(geometry.vertices.data());
/*
    VkDeviceSize vertexBufferSize = sizeof(geometry.vertices[0]) * geometry.vertices.size();

    // copy from staging buffer to the vertex buffer
    resourceManager.getResource<Buffer>(buffers.vertexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(vertexStagingBuffer)->getBuffer(), vertexBufferSize);

    deletionQueue.push(vertexStagingBuffer);
    */

    // Create index buffer
    /*buffers.indexBuffer = resourceManager.createResource<Buffer>(
        "index-buffer", BufferDesc{
            .instanceSize = sizeof(geometry.indices[0]),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .allocationFlags = VMA_MEMORY_USAGE_GPU_ONLY,
        }
    );*/

    // Create index staging buffer
   /*ResourceHandle indexStagingBuffer = resourceManager.createResource<Buffer>(
        "index-staging-buffer", BufferDesc{
            .instanceSize = sizeof(geometry.indices[0]),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });
*/
    // Copy geometry data to the staging buffer
   // resourceManager.getResource<Buffer>(indexStagingBuffer)->map();
   // resourceManager.getResource<Buffer>(indexStagingBuffer)->writeToBuffer(geometry.indices.data());
/*
    VkDeviceSize indexBufferSize = sizeof(geometry.indices[0]) * geometry.indices.size();

    // copy from staging buffer to the index buffer
    resourceManager.getResource<Buffer>(buffers.indexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(indexStagingBuffer)->getBuffer(), indexBufferSize);

    deletionQueue.push(indexStagingBuffer);
*/

    // Create global uniform buffers
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        buffers.global[i] = resourceManager.createResource<Buffer>(
            "global-buffer-" + std::to_string(i), BufferDesc{
                .instanceSize = sizeof(GlobalData),
                .instanceCount = 1,
                .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .queueFamilies = {CommandQueueFamily::Compute, CommandQueueFamily::Graphics},
                .sharingMode = VK_SHARING_MODE_CONCURRENT,
            }
        );
    }
}

void Renderer::createImages() {
    // First, create the default sampler
    images.defaultSampler = resourceManager.createResource<Sampler>("default-sampler", SamplerDesc{});
    // Create all the images
    // Clouds image
    images.cloudsImage = resourceManager.createResource<Image>(
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
    resourceManager.getResource<Image>(images.cloudsImage)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);

    // Clouds mask image
    images.cloudsMaskImage = resourceManager.createResource<Image>(
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
    resourceManager.getResource<Image>(images.cloudsMaskImage)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);
}

void Renderer::loadAssets() {
    // Load the terrain
    //Loader(geometry, device, resourceManager).loadglTF(TERRAIN_GEOMETRY_PATH);

    int w, h, c, d;
    // Low frequency noise
    {
        // Read the data from disk
        ScopedMemory lowFreqNoiseData(readVolume(Filesystem::ls(LOW_FREQ_NOISE_PATH), w, h, c, d,
                                                 Filesystem::ImageFormat::R16G16B16A16_SFLOAT));
        // Create staging buffer
        ResourceHandle lowFreqNoiseStagingBuffer = resourceManager.createResource<Buffer>(
            "base-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(float16_t),
                .instanceCount = static_cast<uint32_t>(w * h * d * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            });
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

        // Submit the staging buffer for deletion
        deletionQueue.push(lowFreqNoiseStagingBuffer);
    }
    // High frequency noise
    {
        // Read the data from disk
        ScopedMemory highFrequencyNoiseData(readVolume(Filesystem::ls(HIGH_FREQ_NOISE_PATH), w, h, c, d,
                                                       Filesystem::ImageFormat::R16G16B16A16_SFLOAT));
        // Create staging buffer
        ResourceHandle highFrequencyNoiseStagingBuffer = resourceManager.createResource<Buffer>(
            "detail-noise-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(float16_t),
                .instanceCount = static_cast<uint32_t>(w * h * d * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        );

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

        // Submit the staging buffer for deletion
        deletionQueue.push(highFrequencyNoiseStagingBuffer);
    }
    // Weather map
    {
        ScopedMemory weatherMapData(readImage(WEATHER_MAP_PATH, w, h, c,
                                              Filesystem::ImageFormat::R8G8B8A8_UNORM));

        // Create host visible staging buffer on device
        ResourceHandle weatherMapStagingBuffer = resourceManager.createResource<Buffer>(
            "weather-staging-buffer",
            BufferDesc{
                .instanceSize = sizeof(uchar8_t),
                .instanceCount = static_cast<uint32_t>(w * h * c),
                .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            }
        );

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

        // Submit the staging buffer for deletion
        deletionQueue.push(weatherMapStagingBuffer);
    }
}

void Renderer::init() {
    loadAssets();

    // Loading assets submitted command buffers that need to finish before we proceed
    device.waitIdle();

    // Create the rest
    createBuffers();
    createImages();
    buildDescriptorSetLayouts();
    buildDescriptorSets();
    //buildPipelines();

    // Wait again
    device.waitIdle();

    // Delete staging buffers
    //processDeletionQueue();
    device.waitIdle();
}

void Renderer::update() {
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

void Renderer::render() {
}
