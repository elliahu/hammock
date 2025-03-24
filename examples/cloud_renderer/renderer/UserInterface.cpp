#include "UserInterface.h"

void ::UserInterface::showCameraWindow() {
    hideAll = true;
    ImGui::SetNextWindowPos(
        ImVec2(window.getExtent().width / 2.0f, window.getExtent().height / 2.0f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::Begin("Camera settings", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

    ImGui::DragFloat3("World space position", cameraRef.position);
    ImGui::SliderFloat("Yaw", cameraRef.yaw, 2*HmckPI, -2*HmckPI);
    ImGui::SliderFloat("Pitch",cameraRef.pitch, HmckPI, -HmckPI);
    ImGui::SliderFloat("Roll", cameraRef.roll, HmckPI, -HmckPI);
    ImGui::SliderFloat("Field of view", cameraRef.fov, HmckToRad(HmckAngleDeg(0.1f)), HmckToRad(HmckAngleDeg(110.0f)));
    ImGui::DragFloat("Draw distance", cameraRef.zfar, 1.f, 0.f);

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        showCamera = false;
        hideAll = false;
    }
    ImGui::End();
}

void ::UserInterface::showPostProcsSettingsWindow() {
    hideAll = true;
    ImGui::SetNextWindowPos(
       ImVec2(window.getExtent().width / 2.0f, window.getExtent().height / 2.0f),
       ImGuiCond_Always,
       ImVec2(0.5f, 0.5f)
   );
    ImGui::Begin("Post processing settings", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );
    ImGui::ColorEdit4("Tint", reinterpret_cast<float *>(postRef.tint));

    // Tone Mapping parameters
    ImGui::SeparatorText("Tone Mapping");
    ImGui::SliderFloat("Exposure", postRef.exposure, -5.0f, 5.0f);
    ImGui::SliderFloat("Gamma", postRef.gamma, 0.5f, 3.0f);
    const char *tonemapItems[] = {"Linear", "Reinhard", "ACES", "Uncharted 2"};
    ImGui::Combo("Tonemap Operator", postRef.tonemapOperator, tonemapItems,
                 IM_ARRAYSIZE(tonemapItems));

    // Color Grading parameters
    ImGui::SeparatorText("Color Grading");
    ImGui::SliderFloat("Contrast", postRef.contrast, 0.5f, 2.0f);
    ImGui::SliderFloat("Brightness", postRef.brightness, -0.5f, 0.5f);
    ImGui::SliderFloat("Saturation", postRef.saturation, 0.0f, 2.0f);

    // Vignette parameters
    ImGui::SeparatorText("Vignette");
    ImGui::SliderFloat("Vignette Strength", postRef.vignetteStrength, 0.0f, 3.0f);
    ImGui::SliderFloat("Vignette Softness", postRef.vignetteSoftness, 0.0f, 2.0f);

    // Effects parameters
    ImGui::SeparatorText("Effects");

    ImGui::SliderFloat("Temperature", postRef.temperature, -1.0f, 1.0f);
    //ImGui::BeginDisabled(true);
    ImGui::SliderFloat("Grain Amount", postRef.grainAmount, 0.0f, 1.0f);

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        showPostProc = false;
        hideAll = false;
    }
    ImGui::End();
}

void ::UserInterface::showDebugWindow() {
    ImGui::Begin("Debug and Performance", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize );

    ImGui::SeparatorText("Performance");
    ImGui::Text("%.1f FPS ", 1.0f / deltaTime);
    ImGui::Text("Frametime: %.2f ms", deltaTime * 1000.0f);
    ImGui::PlotLines("Frame Times", frameTimes, FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex, nullptr,
                     0.0f, 33.0f,
                     ImVec2(0, 80));
    ImGui::SeparatorText("Rendering");
    ImGui::DragInt("Max density samples", cloudsRef.DEBUG_maxSamples, 0.1f, 2, 2048);
    ImGui::DragInt("Max light density samples", cloudsRef.DEBUG_maxLightSamples, 0.1f, 2, 64);
    ImGui::DragInt("Large step multiplier", cloudsRef.DEBUG_longStepMulti, 1, 1, 100);
    ImGui::DragInt("Cheap sample distance", cloudsRef.DEBUG_cheapSampleDistance, 10, 0,
                   1000000);
    ImGui::Checkbox("Enable epic view", (bool *) cloudsRef.DEBUG_epicView);

    ImGui::SeparatorText("Debug views");
    ImGui::Checkbox("Expensive light sampling", (bool *) cloudsRef.DEBUG_expensiveSampling);
    ImGui::Checkbox("Early termination regions", (bool *) cloudsRef.DEBUG_earlyTermination);
    ImGui::Checkbox("Late termination regions", (bool *) cloudsRef.DEBUG_lateTermination);
    ImGui::End();
}

void ::UserInterface::showEditorWindow() {
    ImGui::Begin("Atmosphere editor", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SeparatorText("Clouds");
    ImGui::SliderFloat("Anvil bias", cloudsRef.anvilBias, 0.0f, 1.f);
    ImGui::SliderFloat("Base scale", cloudsRef.baseScale, 0.0f, 200.f);
    ImGui::SliderFloat("Detail scale", cloudsRef.detailScale, 0.0f, 200.f);
    ImGui::SliderFloat("Curls", cloudsRef.curliness, 0.0f, 50.f);
    ImGui::SliderFloat("Low frequency", cloudsRef.baseMultiplier, 0.0f, 1.f);
    ImGui::SliderFloat("High frequency", cloudsRef.detailMultiplier, 0.0f, 1.f);
    ImGui::DragFloat("Absorption", cloudsRef.absorption, 0.0001f, 0.0f, 1.0f, "%.7f");

    ImGui::SeparatorText("Phase");
    ImGui::SliderFloat("Eccentricity", cloudsRef.eccentricity, 0.f, 1.0f);
    ImGui::SliderFloat("Intensity", cloudsRef.intensity, 0.f, 20.0f);
    ImGui::SliderFloat("Spread", cloudsRef.spread, 0.f, 1.0f);

    ImGui::SeparatorText("Weather");
    ImGui::SliderFloat("Global coverage", cloudsRef.globalCoverage, 0.0f, 1.f);
    ImGui::SliderFloat("Global density", cloudsRef.globalDensity, 0.0f, 1.f);
    ImGui::SliderFloat("Wind speed", cloudsRef.cloudSpeed, 0.0f, 5000.f);

    ImGui::SeparatorText("Light");
    ImGui::ColorEdit3("Light color", globalRef.lightColor);
    ImGui::SliderFloat3("Light direction",globalRef.lightDirection, -1.0f, 1.0f);
    ImGui::ColorEdit3("Zenith sky color", globalRef.skyColorZenith);
    ImGui::ColorEdit3("Horizon sky color", globalRef.skyColorHorizon);
    ImGui::SliderFloat("Sun light strength", globalRef.lightColor, 0.0f, 15.f);
    ImGui::SliderFloat("Ambient light strength", cloudsRef.ambientStrength, 0.0f, 1.f);

    ImGui::SeparatorText("Environment properties");
    ImGui::SliderFloat("Time of day", globalRef.timeOfDay, 0.250f, 0.750f);


    ImGui::End();
}

void ::UserInterface::recordUserInterface(VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = frameManager.getSwapChain()->getRenderPass();
    renderPassInfo.framebuffer = frameManager.getSwapChain()->getFramebuffer(frameManager.getSwapChainImageIndex());
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = frameManager.getSwapChain()->getSwapChainExtent();
    VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    // Begin the render pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ui.beginUserInterface();


    ImGui::PushStyleColor(ImGuiCol_Border, {0.f, 0.f, 0.f, 0.f});
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Atmosphere editor", NULL, showEditor)) {
                showEditor = !showEditor;
            }
            if (ImGui::MenuItem("Debug and Performance", NULL, showDebug)) {
                showDebug = !showDebug;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Camera settings", NULL, showCamera)) {
                showCamera = !showCamera;
            }
            if (ImGui::MenuItem("Post processing", NULL, showPostProc)) {
                showPostProc = !showPostProc;
            }
            if (ImGui::MenuItem("Hide all", NULL, hideAll)) {
                hideAll = !hideAll;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleColor();


    if (showEditor && !hideAll) {
        showEditorWindow();
    }

    if (showDebug && !hideAll) {
        showDebugWindow();
    }

    if (showCamera) {
        showCameraWindow();
    }

    if (showPostProc) {
        showPostProcsSettingsWindow();
    }



    ui.endUserInterface(commandBuffer);
    // End the render pass
    vkCmdEndRenderPass(commandBuffer);
}
