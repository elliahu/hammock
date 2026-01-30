#include "imgui/backends/imgui_impl_vulkan.h"
#include "VulkanSurfer/imgui_impl_vulkansurfer.h"
#include "imgui/imgui.h"
#include <compare>
#include "vulkan/vulkan.hpp"

#include "ui.hpp"

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
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;

    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;

    ImGui_ImplVulkan_Init(&initInfo);

    // Create command buffers
    core::SwapChain::forEachFrameInFlight([&, this](int frame) {
        auto commandBuffer = std::make_unique<
            core::CommandBuffer>(ctx->getDevice(), core::CommandQueueFamily::Graphics);
        commandBuffers_.push_back(std::move(commandBuffer));
    });
}

hammock::engine::Ui::~Ui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplVulkanSurfer_Shutdown();
    ImGui::DestroyContext();
}


void hammock::engine::Ui::renderFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &wait, core::Semaphore &signal) {
    // New frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplVulkanSurfer_NewFrame();
    ImGui::NewFrame();

    // Draw data
    draw();
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();

    auto targetImage = core::ResourceManager::getInstance().getResource<core::Image>(target);
    vk::RenderingAttachmentInfo colorAttachment = targetImage->getRenderingAttachmentInfo();
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo renderInfo = {};
    renderInfo.renderArea = vk::Rect2D{{0, 0}, {targetImage->getExtent().width, targetImage->getExtent().height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;

    auto &commandBuffer = *commandBuffers_[frameIndex];

    // Begin command buffer
    commandBuffer.addWaitSemaphore(wait, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    commandBuffer.addSignalSemaphore(signal);
    commandBuffer.begin();

    // Begin rendering
    commandBuffer.getCommandBuffer().beginRendering(&renderInfo);
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.getCommandBuffer());
    commandBuffer.getCommandBuffer().endRendering();

    // Submit
    commandBuffer.submit();
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
