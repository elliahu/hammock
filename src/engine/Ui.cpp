module;

#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui_imp_vulkansurfer.h"
#include "imgui/imgui.h"


module hammock.engine.ui;

hammock::engine::Ui::Ui(Surfer::Window *window, renderer::GraphicsContext *ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplVulkanSurfer_Init(window);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = ctx->getInstance().getInstance();
    init_info.PhysicalDevice = ctx->getDevice().getPhysicalDevice();
    init_info.Device = ctx->getDevice().device();
    init_info.QueueFamily = ctx->getDevice().getGraphicsQueueFamilyIndex();
    init_info.Queue = ctx->getDevice().graphicsQueue();
    init_info.DescriptorPool = core::DescriptorPool::getInstance().getDescriptorPool();
    init_info.MinImageCount = 3; // Usually 2 or 3
    init_info.ImageCount = 3;

    ImGui_ImplVulkan_Init(&init_info);
}

hammock::engine::Ui::~Ui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplVulkanSurfer_Shutdown();
    ImGui::DestroyContext();
}

void hammock::engine::Ui::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplVulkanSurfer_NewFrame();
    ImGui::NewFrame();

    draw();
}

void hammock::engine::Ui::renderFrame(core::CommandBuffer &commandBuffer) {
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.getCommandBuffer());
}

void hammock::engine::Ui::draw() {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

    char buffer[200] = {0};
    ImGui::InputText("label", buffer, 200);

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);
    ImGui::End();
}
