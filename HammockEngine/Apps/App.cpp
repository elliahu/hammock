#include "App.h"

Hmck::App::App()
{
    // TODO change this so that material sets are allocated dynamicly or from different pool object
    globalPool = HmckDescriptorPool::Builder(hmckDevice)
        .setMaxSets(100)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
        .build();

	loadGameObjects();

    globalSetLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();
}

void Hmck::App::run()
{

    std::vector<std::unique_ptr<HmckBuffer>> uboBuffers{ HmckSwapChain::MAX_FRAMES_IN_FLIGHT };
    for (int i = 0; i < uboBuffers.size(); i++)
    {
        uboBuffers[i] = std::make_unique<HmckBuffer>(
            hmckDevice,
            sizeof(HmckGlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
        );
        uboBuffers[i]->map();
    }

    std::vector<VkDescriptorSet> globalDescriptorSets(HmckSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        HmckDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // systems and layouts
    
    std::vector<VkDescriptorSetLayout> globalSetLayouts{
        globalSetLayout->getDescriptorSetLayout(),
    };

    // TODO make HmckGbufferRenderSystem own material set layout 
    std::vector<VkDescriptorSetLayout> gbufferSetLayouts{
        globalSetLayout->getDescriptorSetLayout(),
        materialLayout->getDescriptorSetLayout()
    };

    HmckShadowmapSystem shadowmapRenderSystem{
        hmckDevice,
        hmckRenderer.getOffscreenRenderPass(),
        gbufferSetLayouts
    };

    HmckGbufferRenderSystem gbufferRenderSystem{
        hmckDevice,
        hmckRenderer.getGbufferRenderPass(),
        gbufferSetLayouts
    };

	HmckDeferredRenderSystem deferredRenderSystem{ 
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(), 
        globalSetLayouts
    };

    HmckLightSystem lightSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        globalSetLayouts
    };

    HmckCollisionDetectionSystem collisionDetectionSystem{};

    HmckUISystem userInterfaceSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        hmckWindow
    };


    VkDescriptorImageInfo imageInfo = hmckRenderer.getShadowmapDescriptorImageInfo();
    deferredRenderSystem.updateShadowmapDescriptorSet(imageInfo);

    std::array<VkDescriptorImageInfo, 4> imageInfos{
        hmckRenderer.getGbufferDescriptorImageInfo(0),
        hmckRenderer.getGbufferDescriptorImageInfo(1),
        hmckRenderer.getGbufferDescriptorImageInfo(2),
        hmckRenderer.getGbufferDescriptorImageInfo(3),
    };
    deferredRenderSystem.updateGbufferDescriptorSet(imageInfos);

    // camera and movement
    HmckCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
    auto viewerObject = HmckGameObject::createGameObject();
    viewerObject.transformComponent.translation.z = -2.5f;
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();
	while (!hmckWindow.shouldClose())
	{
        hmckWindow.pollEvents();

        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(hmckWindow, frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transformComponent.translation, viewerObject.transformComponent.rotation);

        float aspect = hmckRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f,  1000.f );

        // start a new frame
		if (auto commandBuffer = hmckRenderer.beginFrame())
		{
            int frameIndex = hmckRenderer.getFrameIndex();
            HmckFrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSets[frameIndex],
                gameObjects
            };

            // UPDATE 
            HmckGlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            lightSystem.update(frameInfo, ubo);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            // RENDER
            // offscreen
            hmckRenderer.beginShadowmapRenderPass(commandBuffer);
            shadowmapRenderSystem.render(frameInfo);
            hmckRenderer.endRenderPass(commandBuffer);

            hmckRenderer.beginGbufferRenderPass(commandBuffer);
            gbufferRenderSystem.render(frameInfo);
            hmckRenderer.endRenderPass(commandBuffer);

            // on screen
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);
			deferredRenderSystem.render(frameInfo);
            // TODO doesn't work because of the depth test failing 
            //lightSystem.render(frameInfo);
        
            // ui
            userInterfaceSystem.beginUserInterface();
            //userInterfaceSystem.showDemoWindow();
            userInterfaceSystem.showDebugStats(viewerObject);
            userInterfaceSystem.showWindowControls();
            userInterfaceSystem.showGameObjectsInspector(gameObjects);
            userInterfaceSystem.endUserInterface(commandBuffer);
            
			hmckRenderer.endRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::App::loadGameObjects()
{
    // materials
    HmckCreateMaterialInfo floorMaterialIfno{};
    floorMaterialIfno.color = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_Color.jpg";
    floorMaterialIfno.normal = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_NormalDX.jpg";
    floorMaterialIfno.roughnessMetalness = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_Roughness.jpg";
    std::shared_ptr<HmckMaterial> floorMaterial = HmckMaterial::createMaterial(hmckDevice, floorMaterialIfno);

    // layouts
    // TODO think about using array of combined image samplers
    // TODO move this to Gbuffer system as it is the only system that uses this, no need for this to be in App
    materialLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    // sphere
    std::shared_ptr<HmckMesh> sphereModel = HmckMesh::createMeshFromObjFile(hmckDevice, std::string(MODELS_DIR) + "sphere.obj");
    auto sphere = HmckGameObject::createGameObject();
    sphere.setName("Sphere");
    sphere.setObjMesh(sphereModel);
    sphere.transformComponent.translation = { 0.0f, -0.5f, 0.f };
    sphere.transformComponent.scale = glm::vec3(.4f);
    sphere.fitBoundingBox(sphereModel->modelInfo);
    sphere.setMtlMaterial(floorMaterial);
    sphere.bindDescriptorSet(globalPool, materialLayout);
    gameObjects.emplace(sphere.getId(), std::move(sphere));

    // floor
    auto floor = HmckGameObject::createFromGLTF(std::string(MODELS_DIR) + "plane/plane.gltf",hmckDevice);
    floor.setName("Floor");
    floor.bindDescriptorSet(globalPool, materialLayout);
    gameObjects.emplace(floor.getId(), std::move(floor));


    // Point lights
    std::vector<glm::vec3> lightColors{
         //{1.f, .1f, .1f},
         //{.1f, .1f, 1.f},
         //{.1f, 1.f, .1f},
         //{1.f, 1.f, .1f},
         //{.1f, 1.f, 1.f},
         {1.f, 1.f, 1.f}, 
    };
    for (int i = 0; i < lightColors.size(); i++)
    {
        auto pointLight = HmckGameObject::createPointLight(1.0f);
        pointLight.setName("Point light");
        pointLight.colorComponent = lightColors[i];
        auto rotateLight = glm::rotate(
            glm::mat4(1.f), 
            (i * glm::two_pi<float>()) / lightColors.size(),
            { 0.f, -1.f, 0.f }
        );
        pointLight.transformComponent.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
    
    // Directional light
    auto directionalLight = HmckGameObject::createDirectionalLight();
    directionalLight.setName("Directional light");
    gameObjects.emplace(directionalLight.getId(), std::move(directionalLight));

    
}