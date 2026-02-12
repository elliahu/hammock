#include "renderer.hpp"

#include <memory>
#include <stdexcept>

#include "core/swapchain.hpp"


hammock::renderer::Renderer::Renderer(core::VulkanContext& context) : ctx_(context) {
    core::SwapChain::forEachFrameInFlight([this](int i) {
        auto commandBuffer =
            std::make_unique<core::CommandBuffer>(*ctx_.device, core::CommandQueueFamily::Graphics);
        commandBuffers_.push_back(std::move(commandBuffer));
    });
}

void hammock::renderer::Renderer::drawFrame(
    core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore& signal) const {
    if (!target.isValid() || target.getType() != core::ResourceType::Image) {
        throw std::runtime_error("Invalid rendering target");
    }

    auto& commandBuffer = *commandBuffers_[frameIndex];
    auto image = ctx_.resourceManager->getResource<core::Image>(target);

    commandBuffer.addSignalSemaphore(signal);
    commandBuffer.begin();

    // Attachment needs to be in color attachment optimal layout
    image->recordPipelineBarrier(commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eNone,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored);

    auto attachment = image->getRenderingAttachmentInfo();
    attachment.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo renderInfo{};
    renderInfo.renderArea = vk::Rect2D{{0, 0}, {image->getExtent().width, image->getExtent().height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attachment;

    commandBuffer.getCommandBuffer().beginRendering(&renderInfo);
    commandBuffer.getCommandBuffer().endRendering();

    commandBuffer.submit();
}
