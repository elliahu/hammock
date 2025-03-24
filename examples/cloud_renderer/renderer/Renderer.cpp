#include "Renderer.h"

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
    allocateCommandBuffers();
    createSyncObjects();

    // Wait again
    device.waitIdle();

    // Delete staging buffers
    processDeletionQueue();

    // Initialize passes
    geometryPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    cloudsPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    compositionPass.setCloudsColor(cloudsPass.getColorTarget());
    compositionPass.setTerrainColor(geometryPass.getColorTarget());
    compositionPass.setTerrainDepth(geometryPass.getDepthTarget());
    compositionPass.initialize(HmckVec2{static_cast<float>(lWidth), static_cast<float>(lHeight)});

    postProcessingPass.setIinput(compositionPass.getColorTarget());
    postProcessingPass.setSwapChainImageFormat(frameManager.getSwapChain()->getSwapChainImageFormat());
    postProcessingPass.initialize();

    ui->setCamera(&camera);
    ui->setCloudsPushData(&cloudsPass.properties);
    ui->setCloudsUniformData(&cloudsPass.uniform);
    ui->setPostProccessingData(&postProcessingPass.data);
}

void Renderer::beginCommandBuffers() {
    // Begin geometry command buffer
    frameManager.beginCommandBuffer(commandBuffers.terrain[frameManager.getFrameIndex()]);
    frameManager.beginCommandBuffer(commandBuffers.composition[frameManager.getFrameIndex()]);
    frameManager.beginCommandBuffer(commandBuffers.clouds[frameManager.getFrameIndex()]);
}

void Renderer::recordSwapChainImageTransition(VkImageLayout from, VkImageLayout to) {
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;


    // Transition the swap chain image into present layout
    transitionImageLayout(commandBuffers.composition[frameManager.getFrameIndex()],
                          frameManager.getSwapChain()->getImage(frameManager.getSwapChainImageIndex()),
                          from, to, subresourceRange);
}


void Renderer::submitCommandBuffers() {
    // Recreate targets if window was resized
    // Note this is quite heavy operation
    // TODO

    // Get current frame index
    uint32_t frameIndex = frameManager.getFrameIndex();

    // Submit clouds command buffer
    frameManager.submitCommandBuffer<CommandQueueFamily::Compute>(
        commandBuffers.clouds[frameIndex], {}, {semaphores.cloudsReady[frameIndex]}, {});

    // Submit terrain command buffer
    frameManager.submitCommandBuffer<CommandQueueFamily::Graphics>(
        commandBuffers.terrain[frameIndex], {}, {semaphores.terrainReady[frameIndex]}, {});

    // Submit composition command buffer
    // This one is submitted for presentation
    // Waits at fragment shader stage on semaphores to be signaled
    frameManager.submitPresentCommandBuffer(commandBuffers.composition[frameIndex],
                                            {semaphores.terrainReady[frameIndex], semaphores.cloudsReady[frameIndex]},
                                            {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT});
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
      lWidth{static_cast<uint32_t>(width)}, lHeight{static_cast<uint32_t>(height)},
      geometryPass(device, resourceManager), cloudsPass(device, resourceManager), compositionPass(device, resourceManager),
      postProcessingPass(device, resourceManager) {
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

    // Update cloud pass
    cloudsPass.setProjection(camera.getProjection());
    cloudsPass.setView(camera.getView());
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

    float angle = (cloudsPass.uniform.timeOfDay - 0.25f) * 2.0f * HmckPI; // Shift so 0.25 (morning) starts at the horizon
    float sunHeight = std::sin(angle); // Vertical movement
    float sunHorizontal = std::cos(angle); // Horizontal movement
    lightDirection = HmckVec4{HmckNorm(HmckVec3{sunHorizontal, sunHeight, 0.0f}), 0.0f};
    cloudsPass.uniform.lightDirection = lightDirection;

    float azimuthRadians = HmckToRad(HmckAngleDeg(45.0f));
    cloudsPass.uniform.windDirection = HmckVec4{HmckCosF(azimuthRadians), 0.0f, HmckSinF(azimuthRadians), 0.0f};


    // Update geometry pass
    geometryPass.setProjection(camera.getProjection());
    geometryPass.setView(camera.getView());
    geometryPass.setLightColor(lightColor.XYZ);
    geometryPass.setLightDirection(lightDirection.XYZ);


    // Update post processing data
    postProcessingPass.setSwapChainImage(frameManager.getSwapChain()->getImage(frameManager.getSwapChainImageIndex()));
    postProcessingPass.setSwapChainImageView(frameManager.getSwapChain()->getImageView(frameManager.getSwapChainImageIndex()));
    postProcessingPass.setSwapChainRenderingExtent(frameManager.getSwapChain()->getSwapChainExtent());
    postProcessingPass.data.time = elapsedTime;
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

            //Begin command buffers
            beginCommandBuffers();

            // Record render passes
            uint32_t frame = frameManager.getFrameIndex();
            geometryPass.setType(GeometryPass::Type::ColorAndDepth);
            geometryPass.recordCommands(commandBuffers.terrain[frame], frame);
            cloudsPass.recordCommands(commandBuffers.clouds[frame], frame);
            compositionPass.recordCommands(commandBuffers.composition[frame], frame);

            recordSwapChainImageTransition(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            postProcessingPass.recordCommands(commandBuffers.composition[frame], frame);
            ui->recordUserInterface(commandBuffers.composition[frame]);
            recordSwapChainImageTransition(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

            // Submit command buffers
            submitCommandBuffers();

            // Submit frame
            frameManager.endFrame();
        }
    }

    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
