#include "SkyScene.h"

void SkyScene::init() {
    // We need to load all the noises and images
    // Image memory is device dedicated (for best performance) - that means it is not accessible by the host directly
    // For each image, we need to create a "staging buffer" that is lives in the device memory and is accessible by the host
    // We need to:
    // 1. Create the staging buffer
    // 2. Copy the image data into the staging buffer
    // 3. Create the actual resource
    // 4. Copy the data from the staging buffer into the resource memory
    // 5. Release the staging buffer

    // Load the base noise
    int w, h, c, d;
    // Read the data from disk
    ScopedMemory baseNoiseData(readVolume(Filesystem::ls(assetPath("noise/base")), w, h, c, d,
                                          Filesystem::ImageFormat::R16G16B16A16_SFLOAT));
    // Create staging buffer
    ResourceHandle baseNoiseStagingBuffer = rm.createResource<Buffer>(
        "base-noise-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float16_t),
            .instanceCount = static_cast<uint32_t>(w * h * d * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );
    // Write data to the buffer
    rm.getResource<Buffer>(baseNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(baseNoiseStagingBuffer)->writeToBuffer(baseNoiseData.get());

    // Create the actual image resource
    compute.baseNoise = rm.createResource<Image>(
        "base-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
        }
    );

    // Copy data from buffer to image
    rm.getResource<Image>(compute.baseNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.baseNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(baseNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(baseNoiseStagingBuffer.getUid());


    // Load the detail noise
    // Read the data from disk
    ScopedMemory detailNoiseData(readVolume(Filesystem::ls(assetPath("noise/detail")), w, h, c, d,
                                            Filesystem::ImageFormat::R16G16B16A16_SFLOAT));
    // Create staging buffer
    ResourceHandle detailNoiseStagingBuffer = rm.createResource<Buffer>(
        "detail-noise-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float16_t),
            .instanceCount = static_cast<uint32_t>(w * h * d * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write data to the buffer
    rm.getResource<Buffer>(detailNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(detailNoiseStagingBuffer)->writeToBuffer(detailNoiseData.get());

    // Create the actual image resource
    compute.detailNoise = rm.createResource<Image>(
        "detail-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
        }
    );

    // Copy data from buffer to image
    rm.getResource<Image>(compute.detailNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.detailNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(detailNoiseStagingBuffer)->getBuffer());

    // Release the staging buffer
    rm.releaseResource(detailNoiseStagingBuffer.getUid());

    // Load the curl noise
    ScopedMemory curlNoiseData(readImage(assetPath("noise/curlNoise.png"), w, h, c,
                                         Filesystem::ImageFormat::R8G8B8A8_UNORM));

    // Create host visible staging buffer on device
    ResourceHandle curlNoiseStagingBuffer = rm.createResource<Buffer>(
        "curl-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(uchar8_t),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(curlNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(curlNoiseStagingBuffer)->writeToBuffer(curlNoiseData.get());

    // Create image resource
    compute.curlNoise = rm.createResource<Image>(
        "curl-noise",
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
    rm.getResource<Image>(compute.curlNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.curlNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(curlNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(curlNoiseStagingBuffer.getUid());

    // Load the cloud map
    ScopedMemory cloudMapData(readImage(assetPath("noise/weatherMap.png"), w, h, c,
                                        Filesystem::ImageFormat::R8G8B8A8_UNORM));

    // Create host visible staging buffer on device
    ResourceHandle cloudMapStagingBuffer = rm.createResource<Buffer>(
        "cloudmap-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(uchar8_t),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(cloudMapStagingBuffer)->map();
    rm.getResource<Buffer>(cloudMapStagingBuffer)->writeToBuffer(cloudMapData.get());

    // Create the image resource
    compute.cloudMap = rm.createResource<Image>(
        "cloud-map",
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
    rm.getResource<Image>(compute.cloudMap)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.cloudMap)->queueCopyFromBuffer(rm.getResource<Buffer>(cloudMapStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Load the sky dome
    ScopedMemory skyDomeData(readImage(assetPath("textures/sky.jpg"), w, h, c,
                                       Filesystem::ImageFormat::R8G8B8A8_UNORM));
    // Create host visible staging buffer on device
    ResourceHandle skyDomeStagingBuffer = rm.createResource<Buffer>(
        "skydome-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(uchar8_t),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(skyDomeStagingBuffer)->map();
    rm.getResource<Buffer>(skyDomeStagingBuffer)->writeToBuffer(skyDomeData.get());

    sky.skyDome = rm.createResource<Image>(
        "skydome",
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
    rm.getResource<Image>(sky.skyDome)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(sky.skyDome)->queueCopyFromBuffer(rm.getResource<Buffer>(skyDomeStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically


    // Other resource are managed by the render graph
    buildRenderGraph();

    // Finally build the pipelines
    buildPipelines();

    // In the constructor if IScene, device is waiting after the initialization so that the queues are finished before rendering
    // No need to do it here again
}

void SkyScene::buildRenderGraph() {
    // First, declare the resources
    // Add the uniform buffer that holds mutable data
    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "camera-ubo", BufferDesc{
            .instanceSize = sizeof(CameraUbo),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });
    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "time-ubo", BufferDesc{
            .instanceSize = sizeof(TimeUbo),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });

    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "sun-and-sky-ubo", BufferDesc{
            .instanceSize = sizeof(SunAndSkyUbo),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });

    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "post-proc-ubo", BufferDesc{
            .instanceSize = sizeof(PostProcessUBO),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });

    // Base noise
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("base-noise", compute.baseNoise);

    // Detail noise
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("detail-noise", compute.detailNoise);

    // Curl noise
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("curl-noise", compute.curlNoise);

    // Cloud map
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("cloud-map", compute.cloudMap);


    // Storage images that the compute pass outputs to and that is then read in the composition pass

    renderGraph->addResource<ResourceNode::Type::StorageImage, Image, ImageDesc>(
        "color-image", ImageDesc{
            .width = window.getExtent().width,
            .height = window.getExtent().height,
            .channels = 4,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        });

    renderGraph->addResource<ResourceNode::Type::ColorAttachment, Image, ImageDesc>(
        "sky-image", ImageDesc{
            .width = window.getExtent().width,
            .height = window.getExtent().height,
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        });


    // Create a default sampler that will be used to sample output images (in this case storage image)
    renderGraph->createSampler("default-sampler");

    // And tell the graph that we will be outputting to swap chain
    renderGraph->addSwapChainImageResource("swap-color-image");

    // Next, declare the render passes and what resources each pass uses

    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("sky-pass")
            .read(ResourceAccess{
                .resourceName = "camera-ubo",
            })
            .read(ResourceAccess{
                .resourceName = "time-ubo",
            })
            .read(ResourceAccess{
                .resourceName = "sun-and-sky-ubo",
            })
            .descriptor(0, {
                            {0, {"camera-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {1, {"time-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {2, {"sun-and-sky-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "sky-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            })
            .execute([&](RenderPassContext context)-> void {
                // The composition pass is straight forward
                sky.pipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, sky.pipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS);


                // Even though there is no vertex buffer, this call is safe as it does not actually read the vertices in the shader
                // This only triggers fullscreen effect in vert shader that runs fragment shader for each pixel of the screen
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });

    // Compute pass reads from storage and uniform buffers and writes into storage image
    renderGraph->addPass<CommandQueueFamily::Compute>("compute-pass")
            .read(ResourceAccess{
                .resourceName = "base-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "detail-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "curl-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "cloud-map",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "camera-ubo",
            })
            .read(ResourceAccess{
                .resourceName = "time-ubo",
            })
            .read(ResourceAccess{
                .resourceName = "sun-and-sky-ubo",
            })
            .read(ResourceAccess{
                .resourceName = "sky-image",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .descriptor(0, {
                            {0, {"camera-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {1, {"time-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {2, {"sun-and-sky-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {3, {"color-image"}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
                            {4, {"base-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {5, {"detail-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {6, {"curl-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {7, {"cloud-map"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {8, {"sky-image"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
            })
            .execute([&](RenderPassContext context)-> void {
                // This is the execution code of the compute pass
                compute.cloudPipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);

                context.get<Buffer>("camera-ubo")->writeToBuffer(&cameraUbo);
                context.get<Buffer>("time-ubo")->writeToBuffer(&timeUbo);
                context.get<Buffer>("sun-and-sky-ubo")->writeToBuffer(&sunAndSkyUbo);

                context.bindDescriptorSet(0, 0, compute.cloudPipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_COMPUTE);

                vkCmdPushConstants(context.commandBuffer, compute.cloudPipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                   sizeof(ComputePushConsts), &computePushConsts);

                // Then we dispatch the compute shader
                // This way we can render in parallel which makes the raymarching way quicker than doing this in frag shader
                vkCmdDispatch(context.commandBuffer, groupsX, groupsY, 1);
            });


    // Composition pass reads from the storage image and writes to the swap chain image
    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("composition-pass")
            .read(ResourceAccess{
                .resourceName = "color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
            })
            .read(ResourceAccess{
                .resourceName = "sky-image",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
            })
            .read(ResourceAccess{
                .resourceName = "post-proc-ubo"
            })
            .descriptor(0, {
                            {0, {"color-image"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {1, {"sky-image"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {2, {"post-proc-ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            })
            .execute([&](RenderPassContext context)-> void {
                // The composition pass is straight forward
                context.get<Buffer>("post-proc-ubo")->writeToBuffer(&postProcUbo);

                composition.pipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, composition.pipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS);


                // Even though there is no vertex buffer, this call is safe as it does not actually read the vertices in the shader
                // This only triggers fullscreen effect in vert shader that runs fragment shader for each pixel of the screen
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });

    // Last is the UI pass
    // Because it uses SwapChain's VkRenderPass and VkFrameBuffer, we mark it as such -> autoBeginRenderingDisabled()
    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("user-interface-pass")
            .autoBeginRenderingDisabled() // This will tell the render graph to not call begin rendering
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Last pass before present
            })
            .execute([this](RenderPassContext context)-> void {
                // Clarification on why calling vkCmdBeginRenderPass:
                // The rendergraph by default uses dynamic rendering but the ImGUI vulkan backend requires valid VkRednerPass object
                // In dynamic rendering workflow, no VkRenderPasses and VkFramebuffer are created or used
                // That is why we need to tell the rendergraph to skip the dynamic rendering in this pass and begin the pass ourselves
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = fm.getSwapChain()->getRenderPass();
                renderPassInfo.framebuffer = fm.getSwapChain()->getFramebuffer(fm.getSwapChainImageIndex());
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = fm.getSwapChain()->getSwapChainExtent();
                VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                // Begin the render pass
                vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Draw the UI
                ui.get()->beginUserInterface();

                static bool showCam = true;
                static bool showPerf = true;
                static bool showPostProc = false;

                // Main menu bar
                if (ImGui::BeginMainMenuBar()) {
                    if (ImGui::BeginMenu("View")) {
                        if (ImGui::MenuItem("Show cloud editor", NULL, showCam)) {
                            showCam = !showCam;
                        }

                        if (ImGui::MenuItem("Show performance overview", NULL, showPerf)) {
                            showPerf = !showPerf;
                        }

                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Options")) {
                        if (ImGui::MenuItem("Post processing", NULL, showPostProc)) {
                            showPostProc = !showPostProc;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
                }


                ImVec2 camWindowPos = ImVec2(0.0f, 0.0f);

                if (showCam) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::SetNextWindowPos({0, static_cast<float>(window.getExtent().height)}, 0, {0, 1});
                    ImGui::Begin("Editor options", (bool *) false,
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);

                    ImGui::SeparatorText("Cloud properties");
                    ImGui::SliderFloat("Coverage override", &computePushConsts.coverageOverride, 0.0f, 1.f);
                    ImGui::SliderFloat("Cloud type override", &computePushConsts.cloudTypeOverride, 0.f, 2.f);
                    ImGui::SliderFloat("Crispiness", &computePushConsts.crispiness, 0.0f, 200.f);
                    ImGui::SliderFloat("Curlines", &computePushConsts.curliness, 0.0f, 50.f);

                    ImGui::DragFloat("Absorption", &computePushConsts.absorption, 0.0001f, 0.0f, 1.0f, "%.7f");
                    ImGui::DragFloat("Scattering", &computePushConsts.scattering, 0.0001f, 0.0f, 1.0f, "%.7f");


                    ImGui::SliderFloat("Eccentricity", &computePushConsts.eccentricity, 0.f, 1.0f);
                    ImGui::SliderFloat("Silver intensity", &computePushConsts.silverIntensity, 0.f, 10.0f);
                    ImGui::SliderFloat("Silver spread", &computePushConsts.silverSpread, 0.f, 1.0f);


                    ImGui::SeparatorText("Noise properties");
                    ImGui::SliderFloat("Base multiplier", &computePushConsts.baseMultiplier, 0.0f, 5.f);
                    ImGui::SliderFloat("Detail multiplier", &computePushConsts.detailMultiplier, 0.0f, 1.f);


                    ImGui::SeparatorText("Environment properties");
                    ImGui::Checkbox("Progress time", &progressTime);
                    ImGui::SliderFloat("Time of day", &timeOfDay, 0.250f, 0.750f);
                    ImGui::SliderFloat("Wind speed", &computePushConsts.cloudSpeed, 0.0f, 1000.f);
                    ImGui::SliderFloat("Wind direction (deg.)", &windDirection, 0.0f, 365.f);
                    ImGui::ColorEdit3("Light color", &sunAndSkyUbo.lightColor.Elements[0]);
                    ImGui::SliderFloat3("Light direction", &sunAndSkyUbo.lightDirection.Elements[0], -1.0f, 1.0f);
                    ImGui::SliderFloat("Ambient light strength", &computePushConsts.ambientStrength, 0.0f, 1.f);
                    ImGui::SliderFloat("Epic distance", &computePushConsts.epicDistance, 0.0f, 500000.0f);

                    if (ImGui::Button("Reset to defaults")) {
                        computePushConsts = backUpComputePushConsts;
                    }

                    camWindowPos = ImGui::GetWindowPos();

                    ImGui::PopStyleVar();
                    ImGui::End();
                }

                if (showPostProc) {
                    // Disable other debug windows when post process is shown
                    showCam = false;
                    showPerf = false;

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    // Center the window on the screen
                    ImGui::SetNextWindowPos(
                        ImVec2(window.getExtent().width / 2.0f, window.getExtent().height / 2.0f),
                        ImGuiCond_Always,
                        ImVec2(0.5f, 0.5f)
                    );
                    ImGui::Begin("Post Process Debug", nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoDecoration);

                    // Color Tint
                    ImGui::SeparatorText("Color Tint");
                    // Assuming HmckVec4 is similar to a glm::vec4 or ImVec4
                    ImGui::ColorEdit4("Tint", reinterpret_cast<float *>(&postProcUbo.colorTint));

                    // Tone Mapping parameters
                    ImGui::SeparatorText("Tone Mapping");
                    ImGui::SliderFloat("Exposure", &postProcUbo.exposure, -5.0f, 5.0f);
                    ImGui::SliderFloat("Gamma", &postProcUbo.gamma, 0.5f, 3.0f);
                    const char *tonemapItems[] = {"Linear", "Reinhard", "ACES", "Uncharted 2"};
                    ImGui::Combo("Tonemap Operator", &postProcUbo.tonemapOperator, tonemapItems, IM_ARRAYSIZE(tonemapItems));

                    // Color Grading parameters
                    ImGui::SeparatorText("Color Grading");
                    ImGui::SliderFloat("Contrast", &postProcUbo.contrast, 0.5f, 2.0f);
                    ImGui::SliderFloat("Brightness", &postProcUbo.brightness, -0.5f, 0.5f);
                    ImGui::SliderFloat("Saturation", &postProcUbo.saturation, 0.0f, 2.0f);

                    // Vignette parameters
                    ImGui::SeparatorText("Vignette");
                    ImGui::SliderFloat("Vignette Strength", &postProcUbo.vignetteStrength, 0.0f, 3.0f);
                    ImGui::SliderFloat("Vignette Softness", &postProcUbo.vignetteSoftness, 0.0f, 2.0f);

                    // Effects parameters
                    ImGui::SeparatorText("Effects");

                    ImGui::SliderFloat("Temperature", &postProcUbo.temperature, -1.0f, 1.0f);
                    //ImGui::BeginDisabled(true);
                    ImGui::SliderFloat("Grain Amount", &postProcUbo.grainAmount, 0.0f, 1.0f);
                    //ImGui::EndDisabled();
                    const char *blendModeItems[] = {"Normal", "Screen", "Soft light"};
                    ImGui::Combo("Cloud blend mode", &postProcUbo.cloudBlendMode, blendModeItems, IM_ARRAYSIZE(blendModeItems));

                    if (ImGui::Button("Close")) {
                        showPostProc = false;
                        showCam = true;
                        showPerf = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset to defaults")) {
                        postProcUbo = backUpPostProcUbo;
                    }


                    ImGui::PopStyleVar();
                    ImGui::End();
                }


                if (showPerf) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::SetNextWindowPos({static_cast<float>(window.getExtent().width), static_cast<float>(window.getExtent().height)},
                                            0, {1, 1});
                    ImGui::Begin("Performance analysis", (bool *) false,
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);
                    ImGui::SeparatorText("Performance");
                    ImGui::Text("%.1f FPS ", 1.0f / deltaTime);
                    ImGui::Text("Frametime: %.2f ms", deltaTime * 1000.0f);
                    ImGui::PlotLines("Frame Times", frameTimes, FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex, nullptr, 0.0f, 33.0f,
                                     ImVec2(0, 80));
                    ImGui::PopStyleVar();
                    ImGui::End();
                }


                ui.get()->endUserInterface(context.commandBuffer);

                // End the render pass
                vkCmdEndRenderPass(context.commandBuffer);
            });

    // Finally, build the rendergraph
    // Rendergraph will prepare resources, optimize the execution and will handle the synchronization
    // This render graph is not re-build every frame like some other implementations
    renderGraph->build();
}

void SkyScene::buildPipelines() {
    // Compute pipeline is not very complicated
    compute.cloudPipeline = ComputePipeline::create({
        .debugName = "compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(compiledShaderPath("clouds.comp")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("compute-pass")},
        .pushConstantRanges{{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConsts)}}
    });

    //sky pass
    sky.pipeline = GraphicsPipeline::create({
        .debugName = "sky-pipeline",
        .device = device,
        .vertexShader
        // Fullscreen vertex shader
        {.byteCode = Filesystem::readFile(compiledShaderPath("atmosphere.vert")),},
        .fragmentShader
        // Fragment shader samples storage texture and writes it to swapchain image
        {.byteCode = Filesystem::readFile(compiledShaderPath("atmosphere.frag")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("sky-pass")},
        .pushConstantRanges{},
        .graphicsState{
            // We disable cull so that the vkCmdDraw command is not skipped
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            // Render graph requires by default dynamic rendering
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {VK_FORMAT_R8G8B8A8_UNORM},
        }
    });

    // Composition pass
    composition.pipeline = GraphicsPipeline::create({
        .debugName = "composition-pipeline",
        .device = device,
        .vertexShader
        // Fullscreen vertex shader
        {.byteCode = Filesystem::readFile(compiledShaderPath("composition.vert")),},
        .fragmentShader
        // Fragment shader samples storage texture and writes it to swapchain image
        {.byteCode = Filesystem::readFile(compiledShaderPath("composition.frag")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("composition-pass")},
        .pushConstantRanges{},
        .graphicsState{
            // We disable cull so that the vkCmdDraw command is not skipped
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            // Render graph requires by default dynamic rendering
            .enabled = true,
            .colorAttachmentCount = 1,
            // We draw to the swapchain image
            .colorAttachmentFormats = {fm.getSwapChain()->getSwapChainImageFormat()},
        }
    });
}

void SkyScene::update() {
    // Timing
    if (progressTime) {
        timeUbo.time += deltaTime;
        postProcUbo.time += deltaTime;
        backUpPostProcUbo.time += deltaTime;
    }
    frameCount++;
    timeUbo.timeOfDay = timeOfDay;
    float angle = (timeOfDay - 0.25f) * 2.0f * HmckPI; // Shift so 0.25 (morning) starts at the horizon
    float sunHeight = std::sin(angle); // Vertical movement
    float sunHorizontal = std::cos(angle); // Horizontal movement

    sunAndSkyUbo.lightDirection = HmckVec4{HmckNorm(HmckVec3{sunHorizontal, sunHeight, 0.0f}), 0.0f}; // Assuming movement in X-Y plane

    float azimuthRadians = HmckToRad(HmckAngleDeg(windDirection));
    sunAndSkyUbo.windDirection = HmckVec4{HmckCosF(azimuthRadians), 0.0f, HmckSinF(azimuthRadians), 0.0f};

    // Movement and rotation speeds (adjust these as needed)
    const float movementSpeed = 500.0f; // Units per frame
    const float rotationSpeed = 2.0f; // Radians per frame

    // Process rotation input using arrow keys.
    // Rotate left/right (yaw)
    if (window.isKeyDown(Surfer::KeyCode::ArrowLeft))
        yaw -= rotationSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowRight))
        yaw += rotationSpeed * deltaTime;

    // Rotate up/down (pitch)
    if (window.isKeyDown(Surfer::KeyCode::ArrowUp))
        pitch += rotationSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowDown))
        pitch -= rotationSpeed * deltaTime;

    // Clamp pitch to avoid excessive rotation (e.g., limit to +/- 89 degrees in radians)
    const float pitchLimit = 1.55334f; // ~89 degrees in radians
    if (pitch > pitchLimit)
        pitch = pitchLimit;
    if (pitch < -pitchLimit)
        pitch = -pitchLimit;

    // Compute the forward direction vector from yaw and pitch.
    const float cosPitch = std::cos(pitch);
    const float sinPitch = std::sin(pitch);
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);
    const HmckVec3 direction = HmckVec3{cosYaw * cosPitch, sinPitch, sinYaw * cosPitch};

    // Get the up vector from the projection (assumed constant)
    const HmckVec3 up = Projection().upPosY();

    // Compute the right vector (perpendicular to both direction and up)
    const HmckVec3 right = HmckNorm(HmckCross(direction, up));

    // Process translation input (WASD keys)
    if (window.isKeyDown(Surfer::KeyCode::KeyW))
        cameraPosition += direction * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyS))
        cameraPosition -= direction * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyA))
        cameraPosition -= right * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyD))
        cameraPosition += right * movementSpeed * deltaTime;

    if (window.isKeyDown(Surfer::KeyCode::Space))
        cameraPosition += up * movementSpeed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::LeftShift))
        cameraPosition -= up * movementSpeed * deltaTime;

    // Compute the target point from the camera position and forward direction.
    const HmckVec3 target = cameraPosition + direction;

    // Create the inverse view matrix based on the updated camera parameters.
    const HmckMat4 view = Projection().view(cameraPosition, target, up);
    const HmckMat4 proj = Projection().perspective(HmckToRad(fov), fm.getAspectRatio(), 0.01, 1000, false);


    cameraUbo.invView = HmckInvGeneral(view);
    cameraUbo.invProj = HmckInvGeneral(proj);
    cameraUbo.invViewProj = HmckInvGeneral(proj * view);
    cameraUbo.cameraPosition = HmckVec4{cameraPosition, 0.0f};
    cameraUbo.resX = static_cast<float>(window.getExtent().width);
    cameraUbo.resY = static_cast<float>(window.getExtent().height);
    cameraUbo.fov = fov;
}


void SkyScene::render() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        // Timing
        auto newTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        if (progressTime) totalElapsedTime += deltaTime;
        currentTime = newTime;

        // Update frame time tracking
        frameTimes[frameTimeFrameIndex] = deltaTime * 1000.0f;
        frameTimeFrameIndex = (frameTimeFrameIndex + 1) % FRAMETIME_BUFFER_SIZE;

        // Poll for events
        window.pollEvents();

        // Update the data for the frame
        update();

        // Execute the render graph
        renderGraph->execute();
    }
    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
