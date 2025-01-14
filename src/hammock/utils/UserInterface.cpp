#include "hammock/utils/UserInterface.h"
#include "hammock/core/GraphicsPipeline.h"
#include "backends/imgui_impl_vulkan.h"
#include "hammock/utils/Helpers.h"
#include <deque>
#include <string>


Hammock::UserInterface::UserInterface(Device &device, const VkRenderPass renderPass,VkDescriptorPool descriptorPool, Window &window) : device{device},
    window{window}, renderPass{renderPass}, imguiPool{descriptorPool} {
    init();
    setupStyle();
}

Hammock::UserInterface::~UserInterface() {
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}

void Hammock::UserInterface::beginUserInterface() {
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void Hammock::UserInterface::endUserInterface(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Hammock::UserInterface::showDebugStats(const HmckMat4 &inverseView, float frameTime) {
    const ImGuiIO &io = ImGui::GetIO();
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                              ImGuiWindowFlags_NoNav;
    ImGui::SetNextWindowPos({10, 10});
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin(window.getWindowName().c_str(), (bool *) nullptr, window_flags);
    const auto cameraPosition = inverseView.Columns[3].XYZ;

    ImGui::Text("Camera world position: ( %.2f, %.2f, %.2f )", cameraPosition.X, cameraPosition.Y, cameraPosition.Z);
    if (ImGui::IsMousePosValid())
        ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
    else
        ImGui::Text("Mouse Position: <invalid or hidden>");
    ImGui::Text("Window resolution: (%d x %d)", window.getExtent().width, window.getExtent().height);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", frameTime * 1000.f,  1.0f / frameTime );
    ImGui::End();
}


void Hammock::UserInterface::showColorSettings(float *exposure, float *gamma, float *whitePoint) {
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    beginWindow("Color settings", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat("Exposure", exposure, 0.01f, 0.01f);
    ImGui::DragFloat("Gamma", gamma, 0.01f, 0.01f);
    ImGui::DragFloat("White point", whitePoint, 0.01f, 0.01f);
    endWindow();
}

void Hammock::UserInterface::init() {
    ImGui::CreateContext();

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.device();
    init_info.Queue = device.graphicsQueue();
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = renderPass;

    ImGui_ImplVulkan_Init(&init_info);

    const ImVec2 display_size(window.getExtent().width, window.getExtent().height);
    ImGui::GetIO().DisplaySize = display_size;

    //execute a gpu command to upload imgui font textures
    ImGui_ImplVulkan_CreateFontsTexture();
}

void Hammock::UserInterface::setupStyle() {
    ImGuiStyle &style = ImGui::GetStyle();
    // Setup ImGUI style

    // Rounding
    style.Alpha = .9f;
    float globalRounding = 9.0f;
    style.FrameRounding = globalRounding;
    style.WindowRounding = globalRounding;
    style.ChildRounding = globalRounding;
    style.PopupRounding = globalRounding;
    style.ScrollbarRounding = globalRounding;
    style.GrabRounding = globalRounding;
    style.LogSliderDeadzone = globalRounding;
    style.TabRounding = globalRounding;

    // padding
    style.WindowPadding = {10.0f, 10.0f};
    style.FramePadding = {3.0f, 5.0f};
    style.ItemSpacing = {6.0f, 6.0f};
    style.ItemInnerSpacing = {6.0f, 6.0f};
    style.ScrollbarSize = 8.0f;
    style.GrabMinSize = 10.0f;

    // align
    style.WindowTitleAlign = {.5f, .5f};

    // colors
    style.Colors[ImGuiCol_TitleBg] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(48 / 255.0f, 48 / 255.0f, 54 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(48 / 255.0f, 48 / 255.0f, 54 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(48 / 255.0f, 48 / 255.0f, 54 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(33 / 255.0f, 33 / 255.0f, 33 / 255.0f, 50 / 255.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(80 / 255.0f, 80 / 255.0f, 80 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
}

VkCommandBuffer Hammock::UserInterface::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Hammock::UserInterface::endSingleTimeCommands(const VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.graphicsQueue());

    vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1, &commandBuffer);
}

void Hammock::UserInterface::beginWindow(const char *title, bool *open, ImGuiWindowFlags flags) {
    ImGui::Begin(title, open, flags);
}

void Hammock::UserInterface::endWindow() {
    ImGui::End();
}
