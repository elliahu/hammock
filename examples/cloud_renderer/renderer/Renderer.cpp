#include "Renderer.h"

#include <future>

void Renderer::processDeletionQueue() {
    while (!deletionQueue.empty()) {
        auto item = deletionQueue.front();
        resourceManager.releaseResource(item.getUid());
        deletionQueue.pop();
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
    ASSERT(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.depth.data()) == VK_SUCCESS,
           "Failed to allocate depth command buffers!");
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

    vkFreeCommandBuffers(
        device.device(),
        device.getGraphicsCommandPool(),
        static_cast<uint32_t>(commandBuffers.depth.size()),
        commandBuffers.depth.data());

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
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.depthReady[i]) == VK_SUCCESS,
               "Failed to create depth ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.cloudsReady[i]) == VK_SUCCESS,
               "Failed to create clouds ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.atmosphereReady[i]) == VK_SUCCESS,
               "Failed to create atmosphere ready semaphore!");
        ASSERT(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &semaphores.terrainColorReady[i]) == VK_SUCCESS,
               "Failed to create terrain ready semaphore!");
    }
}

void Renderer::destroySyncObjects() {
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device(), semaphores.depthReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.cloudsReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.atmosphereReady[i], nullptr);
        vkDestroySemaphore(device.device(), semaphores.terrainColorReady[i], nullptr);
    }
}

void Renderer::prepareGeometry() {
    Loader(geometry, device, resourceManager).loadglTF(ASSET_PATH("mountain.glb"));

    ASSERT(!geometry.vertices.empty(), "No vertices loaded!");

    vertexBuffer = resourceManager.createResource<Buffer>(
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
    resourceManager.getResource<Buffer>(vertexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(vertexStagingBuffer)->getBuffer(), vertexBufferSize);
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->unmap();

    // Create index buffer
    indexBuffer = resourceManager.createResource<Buffer>(
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
    resourceManager.getResource<Buffer>(indexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(indexStagingBuffer)->getBuffer(), indexBufferSize);
    resourceManager.getResource<Buffer>(indexStagingBuffer)->unmap();
}

void Renderer::init() {
    allocateCommandBuffers();
    createSyncObjects();
    prepareGeometry();

    // Initialize threadpool
    processorCount = std::thread::hardware_concurrency();
    ASSERT(processorCount > 0, "Failed to detect number of processors!");
    Logger::log(LOG_LEVEL_DEBUG, "Using %d threads\n", processorCount);
    threadPool.setThreadCount(processorCount);

    // Wait again
    device.waitIdle();

    // Delete staging buffers
    processDeletionQueue();

    // Initialize passes
    depthPass.setVertexBuffer(resourceManager.getResource<Buffer>(vertexBuffer));
    depthPass.setIndexBuffer(resourceManager.getResource<Buffer>(indexBuffer));
    depthPass.initialize(HmckVec2{(float) lWidth, (float) lHeight});

    atmospherePass.setShadowMap(depthPass.getSunDepth());
    atmospherePass.initialize();


    geometryPass.setVertexBuffer(resourceManager.getResource<Buffer>(vertexBuffer));
    geometryPass.setIndexBuffer(resourceManager.getResource<Buffer>(indexBuffer));
    geometryPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    cloudsPass.setCameraDepth(depthPass.getCameraDepth());
    cloudsPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    godRaysPass.setCloudsImage(cloudsPass.getColorTarget());
    godRaysPass.setTerrainDepth(geometryPass.getDepthTarget());
    godRaysPass.initialize(HmckVec2{(float) lWidth, (float) lHeight});

    compositionPass.setCloudsColor(cloudsPass.getColorTarget());
    compositionPass.setTerrainColor(geometryPass.getColorTarget());
    compositionPass.setTerrainDepth(geometryPass.getDepthTarget());
    compositionPass.setTransmittanceLUT(atmospherePass.transmittance.getLut());
    compositionPass.setSkyViewLUT(atmospherePass.skyView.getLut());
    compositionPass.setAerialPerspectiveLUT(atmospherePass.aerialPerspective.getLut());
    compositionPass.setSunShadow(depthPass.getSunDepth());
    compositionPass.setGodRaysTexture(godRaysPass.getGodRaysTexture());
    compositionPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    postProcessingPass.setIinput(compositionPass.getColorTarget());
    postProcessingPass.setSwapChainImageFormat(frameManager.getSwapChain()->getSwapChainImageFormat());
    postProcessingPass.initialize();


    ui->setCamera(&camera);
    ui->setCloudsPushData(&cloudsPass.properties);
    ui->setCloudsUniformData(&cloudsPass.uniform);
    ui->setPostProccessingData(&postProcessingPass.data);
}


void Renderer::recordSwapChainImageTransition(VkImageLayout from, VkImageLayout to, uint32_t frameIndex, uint32_t imageIndex) {
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;


    // Transition the swap chain image into present layout
    transitionImageLayout(commandBuffers.composition[frameIndex],
                          frameManager.getSwapChain()->getImage(imageIndex),
                          from, to, subresourceRange);
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
      device{instance, window.getSurface()}, resourceManager{device}, frameManager{window, device},
      lWidth{static_cast<uint32_t>(width)}, lHeight{static_cast<uint32_t>(height)}, depthPass(device, resourceManager, geometry),
      geometryPass(device, resourceManager, geometry), cloudsPass(device, resourceManager), atmospherePass(device, resourceManager),
      godRaysPass(device, resourceManager),
      compositionPass(device, resourceManager), postProcessingPass(device, resourceManager) {
    // Initialize the descriptor pool object from which descriptors will be allocated
    descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .build();
    ui = std::make_unique<::UserInterface>(device, frameManager, descriptorPool->descriptorPool, window, deltaTime, frameTimes,
                                           FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex);
    init();
}

Renderer::~Renderer() {
    destroyCommandBuffers();
    destroySyncObjects();
}

void Renderer::update() {
    // Update camera
    camera.aspect = frameManager.getAspectRatio();

    HmckMat4 projection = camera.getProjection();
    HmckMat4 view = camera.getView();
    HmckMat4 inverseProjection = HmckInvGeneral(projection);
    HmckMat4 inverseView = HmckInvGeneral(view);
    Camera::FrustumDirections frustum = camera.getFrustumDirections();

    // Update cloud pass
    cloudsPass.setProjection(projection);
    cloudsPass.setView(view);
    cloudsPass.uniform.fov = HmckToDeg(HmckAngleRad(camera.fov));
    cloudsPass.uniform.cameraPosition = HmckVec4{camera.position, 0.0f};
    cloudsPass.uniform.resX = lWidth;
    cloudsPass.uniform.resY = lHeight;
    cloudsPass.uniform.znear = camera.znear;
    cloudsPass.uniform.zfar = camera.zfar;
    cloudsPass.uniform.frameIndexMod16 = frameIndex % 16;
    if (progressTime) {
        cloudsPass.uniform.time = elapsedTime;
    }

    float azimuthRadians = HmckToRad(HmckAngleDeg(45.0f));
    cloudsPass.uniform.windDirection = HmckVec4{HmckCosF(azimuthRadians), 0.0f, HmckSinF(azimuthRadians), 0.0f};


    // Update geometry pass
    geometryPass.setProjection(projection);
    geometryPass.setView(camera.getView());
    geometryPass.setLightColor(cloudsPass.uniform.lightColor.XYZ);
    geometryPass.setLightDirection(cloudsPass.uniform.lightDirection.XYZ);


    // Update post processing data
    postProcessingPass.setSwapChainImage(frameManager.getSwapChain()->getImage(frameManager.getSwapChainImageIndex()));
    postProcessingPass.setSwapChainImageView(frameManager.getSwapChain()->getImageView(frameManager.getSwapChainImageIndex()));
    postProcessingPass.setSwapChainRenderingExtent(frameManager.getSwapChain()->getSwapChainExtent());
    postProcessingPass.data.time = elapsedTime;

    // Update atmosphere
    atmospherePass.setEye(camera.position);
    atmospherePass.setSunDirection(cloudsPass.uniform.lightDirection.XYZ);

    HmckMat4 shadowProjection = Projection().orthographic(-120.0, 120.0, 120.0, -120.0, camera.znear, camera.zfar, true);
    HmckMat4 shadowView = HmckLookAt_RH(cloudsPass.uniform.lightDirection.XYZ * 200.0, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    HmckMat4 shadowViewProjection = shadowProjection * shadowView;

    atmospherePass.setShadowViewProjection(shadowViewProjection);
    atmospherePass.setCameraInverseProjection(inverseProjection);
    atmospherePass.setCameraInverseView(inverseView);
    atmospherePass.setCameraFrustum(
        HmckVec4{frustum.frustumA, 0.0f},
        HmckVec4{frustum.frustumB, 0.0f},
        HmckVec4{frustum.frustumC, 0.0f},
        HmckVec4{frustum.frustumD, 0.0f}
    );

    // Update depth pass
    depthPass.setCameraProjection(projection);
    depthPass.setCameraView(view);

    depthPass.setSunProjection(shadowProjection);
    depthPass.setSunView(shadowView);

    HmckVec3 simulatedSunPos = camera.position - cloudsPass.uniform.lightDirection.XYZ * 1000.0f;
    HmckVec4 clipSpaceSunPos = projection * view * HmckVec4{simulatedSunPos.X, simulatedSunPos.Y, simulatedSunPos.Z, 1.0f};
    HmckVec3 ndcSunPos = {
        clipSpaceSunPos.X / clipSpaceSunPos.W,
        clipSpaceSunPos.Y / clipSpaceSunPos.W,
        clipSpaceSunPos.Z / clipSpaceSunPos.W
    };

    // Convert NDC [-1,1] to screen space [0,1]
    HmckVec2 screenSpaceSunPos = {
        (ndcSunPos.X + 1.0f) * 0.5f,
        (ndcSunPos.Y + 1.0f) * 0.5f
    };

    godRaysPass.setSunScreenSpacePosition(screenSpaceSunPos.X, screenSpaceSunPos.Y);

    compositionPass.setInvView(inverseView);
    compositionPass.setInvProjection(inverseProjection);
    compositionPass.setShadowViewProj(shadowViewProjection);
    compositionPass.setSunDirection(cloudsPass.uniform.lightDirection);
    compositionPass.setSunColor(cloudsPass.uniform.lightColor);
    compositionPass.setAmbientColor(cloudsPass.uniform.skyColorZenith);
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
        frameIndex++;

        // Record and submit frame
        if (frameManager.beginFrame()) {
            // Update the data for the frame
            update();

            // Record render passes
            uint32_t frame = frameManager.getFrameIndex();
            uint32_t image = frameManager.getSwapChainImageIndex();


            // Depth pass
            frameManager.beginCommandBuffer(commandBuffers.depth[frame]);
            depthPass.recordCommands(commandBuffers.depth[frame], frame);
            // Submit depth command buffer
            frameManager.submitCommandBuffer<CommandQueueFamily::Graphics>(
                commandBuffers.depth[frame], {}, {semaphores.depthReady[frame]}, {});


            // Clouds pass
            frameManager.beginCommandBuffer(commandBuffers.clouds[frame]);
            cloudsPass.recordCommands(commandBuffers.clouds[frame], frame);
            // Submit clouds command buffer
            frameManager.submitCommandBuffer<CommandQueueFamily::Compute>(
                commandBuffers.clouds[frame], {semaphores.depthReady[frame]}, {semaphores.cloudsReady[frame]},
                {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT});

            // Atmosphere pass
            frameManager.beginCommandBuffer(commandBuffers.atmosphere[frame]);
            atmospherePass.recordCommands(commandBuffers.atmosphere[frame], frame);
            // Submit atmosphere command buffers
            frameManager.submitCommandBuffer<CommandQueueFamily::Compute>(
                commandBuffers.atmosphere[frame], {}, {semaphores.atmosphereReady[frame]},
                {});

            // Geometry pass
            frameManager.beginCommandBuffer(commandBuffers.terrain[frame]);
            geometryPass.recordCommands(commandBuffers.terrain[frame], frame);
            // Submit terrain command buffer
            frameManager.submitCommandBuffer<CommandQueueFamily::Graphics>(
                commandBuffers.terrain[frame], {}, {semaphores.terrainColorReady[frame]}, {});

            // Composition pass & post process & ui
            frameManager.beginCommandBuffer(commandBuffers.composition[frame]);
            godRaysPass.recordCommands(commandBuffers.composition[frame], frame);
            compositionPass.recordCommands(commandBuffers.composition[frame], frame);
            recordSwapChainImageTransition(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, frame, image);
            postProcessingPass.recordCommands(commandBuffers.composition[frame], frame);
            ui->recordUserInterface(commandBuffers.composition[frame]);
            recordSwapChainImageTransition(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, frame, image);
            // Submit composition command buffer
            // This one is submitted for presentation
            // Waits at fragment shader stage on semaphores to be signaled
            frameManager.submitPresentCommandBuffer(commandBuffers.composition[frame],
                                                    {
                                                        semaphores.terrainColorReady[frame],
                                                        semaphores.cloudsReady[frame],
                                                        semaphores.atmosphereReady[frame]
                                                    },
                                                    {
                                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                                    });

            // Submit frame
            frameManager.endFrame();
        }
    }

    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
