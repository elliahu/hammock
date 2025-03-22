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

    ImGui::DragFloat3("World space position", &camera.position.Elements[0]);
    ImGui::SliderFloat("Yaw", &camera.yaw, 2*HmckPI, -2*HmckPI);
    ImGui::SliderFloat("Pitch", &camera.pitch, HmckPI, -HmckPI);
    ImGui::SliderFloat("Roll", &camera.roll, HmckPI, -HmckPI);
    ImGui::SliderFloat("Field of view", &camera.fov, HmckToRad(HmckAngleDeg(0.1f)), HmckToRad(HmckAngleDeg(179.0f)));
    ImGui::DragFloat("Draw distance", &camera.far, 1.f, 0.f);

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
    ImGui::ColorEdit4("Tint", reinterpret_cast<float *>(&postProcessingData.colorTint));

    // Tone Mapping parameters
    ImGui::SeparatorText("Tone Mapping");
    ImGui::SliderFloat("Exposure", &postProcessingData.exposure, -5.0f, 5.0f);
    ImGui::SliderFloat("Gamma", &postProcessingData.gamma, 0.5f, 3.0f);
    const char *tonemapItems[] = {"Linear", "Reinhard", "ACES", "Uncharted 2"};
    ImGui::Combo("Tonemap Operator", &postProcessingData.tonemapOperator, tonemapItems,
                 IM_ARRAYSIZE(tonemapItems));

    // Color Grading parameters
    ImGui::SeparatorText("Color Grading");
    ImGui::SliderFloat("Contrast", &postProcessingData.contrast, 0.5f, 2.0f);
    ImGui::SliderFloat("Brightness", &postProcessingData.brightness, -0.5f, 0.5f);
    ImGui::SliderFloat("Saturation", &postProcessingData.saturation, 0.0f, 2.0f);

    // Vignette parameters
    ImGui::SeparatorText("Vignette");
    ImGui::SliderFloat("Vignette Strength", &postProcessingData.vignetteStrength, 0.0f, 3.0f);
    ImGui::SliderFloat("Vignette Softness", &postProcessingData.vignetteSoftness, 0.0f, 2.0f);

    // Effects parameters
    ImGui::SeparatorText("Effects");

    ImGui::SliderFloat("Temperature", &postProcessingData.temperature, -1.0f, 1.0f);
    //ImGui::BeginDisabled(true);
    ImGui::SliderFloat("Grain Amount", &postProcessingData.grainAmount, 0.0f, 1.0f);

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        showPostProc = false;
        hideAll = false;
    }
    ImGui::End();
}

void ::UserInterface::showDebugWindow() {
    ImGui::Begin("Performance analysis", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize );

    ImGui::SeparatorText("Performance");
    ImGui::Text("%.1f FPS ", 1.0f / deltaTime);
    ImGui::Text("Frametime: %.2f ms", deltaTime * 1000.0f);
    ImGui::PlotLines("Frame Times", frameTimes, FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex, nullptr,
                     0.0f, 33.0f,
                     ImVec2(0, 80));

    ImGui::End();
}

void ::UserInterface::showEditorWindow() {
    ImGui::Begin("Cloud editor", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SeparatorText("Clouds");
    ImGui::SeparatorText("Light");
    ImGui::ColorEdit3("Light color", &globalData.lightColor.Elements[0]);
    ImGui::SliderFloat3("Light direction", &globalData.lightDirection.Elements[0], -1.0f, 1.0f);
    ImGui::ColorEdit3("Zenith sky color", &globalData.skyColorZenith.Elements[0]);
    ImGui::ColorEdit3("Horizon sky color", &globalData.skyColorHorizon.Elements[0]);
    ImGui::SliderFloat("Sun light strength", &globalData.lightColor.A, 0.0f, 15.f);


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
            if (ImGui::MenuItem("Editor", NULL, showEditor)) {
                showEditor = !showEditor;
            }
            if (ImGui::MenuItem("Debug", NULL, showDebug)) {
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
