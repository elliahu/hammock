#include "UserInterface.h"

void ::UserInterface::showDebugWindow() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos({
                                static_cast<float>(window.getExtent().width),
                                static_cast<float>(window.getExtent().height)
                            },
                            0, {1, 1});
    ImGui::Begin("Performance analysis", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);

    ImGui::SeparatorText("Performance");
    ImGui::Text("%.1f FPS ", 1.0f / deltaTime);
    ImGui::Text("Frametime: %.2f ms", deltaTime * 1000.0f);
    ImGui::PlotLines("Frame Times", frameTimes, FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex, nullptr,
                     0.0f, 33.0f,
                     ImVec2(0, 80));

    ImGui::PopStyleVar();
    ImGui::End();
}

void ::UserInterface::showCloudsWindow() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos({0, static_cast<float>(window.getExtent().height)}, 0, {0, 1});
    ImGui::Begin("Editor options", (bool *) false,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);

    ImGui::SeparatorText("Clouds");
    ImGui::SeparatorText("Light");
    ImGui::ColorEdit3("Light color", &globalData.lightColor.Elements[0]);
    ImGui::SliderFloat3("Light direction", &globalData.lightDirection.Elements[0], -1.0f, 1.0f);
    ImGui::ColorEdit3("Zenith sky color", &globalData.skyColorZenith.Elements[0]);
    ImGui::ColorEdit3("Horizon sky color", &globalData.skyColorHorizon.Elements[0]);
    ImGui::SliderFloat("Sun light strength", &globalData.lightColor.A, 0.0f, 15.f);


    ImGui::PopStyleVar();
    ImGui::End();

    if (showCamera) {
        hideAll = true;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        // Center the window on the screen
        ImGui::SetNextWindowPos(
            ImVec2(window.getExtent().width / 2.0f, window.getExtent().height / 2.0f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.5f)
        );
        ImGui::Begin("Camera settings", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoDecoration);

        ImGui::SeparatorText("Camera settings");
        ImGui::DragFloat3("Position", &camera.position.Elements[0]);

        ImGui::Separator();
        if (ImGui::Button("Close")) {
            showCamera = false;
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }
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

    // Set up window size
    ImVec2 windowSize(frameManager.getSwapChain()->getSwapChainExtent().width,
                      frameManager.getSwapChain()->getSwapChainExtent().height);


    ImGui::PushStyleColor(ImGuiCol_Border, {0.f, 0.f, 0.f, 0.f});
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Atmosphere", NULL, showAtmosphere)) {
                showAtmosphere = !showAtmosphere;
            }
            if (ImGui::MenuItem("Clouds & god rays", NULL, showClouds)) {
                showClouds = !showClouds;
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


    if (showClouds && !hideAll) {
        showCloudsWindow();
    }

    if (showDebug) {
        showDebugWindow();
    }


    ui.endUserInterface(commandBuffer);
    // End the render pass
    vkCmdEndRenderPass(commandBuffer);
}
