module;

#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui_impl_vulkansurfer.h"
#include "imgui/imgui.h"
#include <compare>
#include "vulkan/vulkan.hpp"


module hammock.engine.ui;

hammock::engine::Ui::Ui(Surfer::Window *window, renderer::GraphicsContext *ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplVulkanSurfer_Init(window);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = ctx->getInstance().getInstance();
    initInfo.PhysicalDevice = ctx->getDevice().getPhysicalDevice();
    initInfo.Device = ctx->getDevice().device();
    initInfo.QueueFamily = ctx->getDevice().getGraphicsQueueFamilyIndex();
    initInfo.Queue = ctx->getDevice().graphicsQueue();
    initInfo.DescriptorPool = core::DescriptorPool::getInstance().getDescriptorPool();
    initInfo.MinImageCount = 3; // Usually 2 or 3
    initInfo.ImageCount = 3;
    initInfo.UseDynamicRendering = true;
    // Set up dynamic rendering info
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM; // Use your swapchain format here

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;

    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;

    ImGui_ImplVulkan_Init(&initInfo);
}

hammock::engine::Ui::~Ui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplVulkanSurfer_Shutdown();
    ImGui::DestroyContext();
}


void hammock::engine::Ui::renderFrame(core::CommandBuffer &commandBuffer, vk::ImageView swapchainImageView, std::uint32_t width, std::uint32_t height) {
    // New frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplVulkanSurfer_NewFrame();
    ImGui::NewFrame();

    // Draw data
    draw();
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = swapchainImageView;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad; // Keep existing content
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo renderInfo = {};
    renderInfo.renderArea = vk::Rect2D{{.x = 0, .y = 0}, {.width = width, .height = height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;

    commandBuffer.getCommandBuffer().beginRendering(&renderInfo);
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.getCommandBuffer());
    commandBuffer.getCommandBuffer().endRendering();
}

void hammock::engine::Ui::draw() {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");

    ImGui::Text("This is some useful text.");

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

    char buffer[200] = {0};
    ImGui::InputText("label", buffer, 200);

    if (ImGui::Button("Button"))
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);
    ImGui::End();
}
