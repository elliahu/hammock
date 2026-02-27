#include "renderer.hpp"

#include <memory>
#include <stdexcept>

#include "buffer.hpp"
#include "core/swapchain.hpp"
#include "descriptors.hpp"
#include "utils/filesystem.hpp"
#include "ui/font.hpp"
#include "graphics_pipeline.hpp"
#include "image.hpp"
#include "ui/ui.hpp"
#include "vertex.hpp"
#include "vulkan/vulkan.hpp"

hammock::renderer::Renderer::Renderer(core::VulkanContext& context) : ctx_(context) {
    core::SwapChain::forEachFrameInFlight([this](int i) {
        auto commandBuffer =
            std::make_unique<core::CommandBuffer>(*ctx_.device, core::CommandQueueFamily::Graphics);
        commandBuffers_.push_back(std::move(commandBuffer));
    });

    // Create font atlas GPU resource
    ui::FontAtlas& atlas = ui::Ui::instance().getFontAtlas();
    fontAtlasHandle =
        context.resourceManager->createResource<core::Image>(core::ImageDesc{.width = ui::ATLAS_WIDTH,
            .height = ui::ATLAS_HEIGHT,
            .channels = 1,
            .format = core::ImageFormat::R8Uint,
            .usage = core::ImageUsage::Sampled | core::ImageUsage::TransferDst,
            .type = core::ImageType::Type2D});
    core::Image* fontAtlas = context.resourceManager->getResource<core::Image>(fontAtlasHandle);
    fontAtlas->createSampler();
    core::ResourceHandle atlasStagingBufferHandle = context.resourceManager->createResource<core::Buffer>(
        core::BufferDesc{.type = core::BufferType::HostVisible,
            .usage = core::BufferUsage::TransferSrc | core::BufferUsage::TransferDst,
            .instanceSize = sizeof(uint8_t),
            .instanceCount = ui::ATLAS_WIDTH * ui::ATLAS_HEIGHT});
    core::Buffer* atlasStagingBuffer =
        context.resourceManager->getResource<core::Buffer>(atlasStagingBufferHandle);
    atlasStagingBuffer->map();
    atlasStagingBuffer->writeToBuffer(atlas.bitmap.data());
    fontAtlas->queueImageLayoutTransition(vk::ImageLayout::eTransferDstOptimal);
    fontAtlas->queueCopyFromBuffer(*atlasStagingBuffer);
    fontAtlas->queueImageLayoutTransition(vk::ImageLayout::eShaderReadOnlyOptimal);
    context.device->waitIdle();
    atlasStagingBuffer->unmap();
    context.resourceManager->releaseResource(atlasStagingBufferHandle.getUid());

    // Create font atlas descriptor
    userInterfaceDescLayout_ =
        core::DescriptorSetLayoutBuilder(*context.device)
            .addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
            .build();
    auto imageInfo = fontAtlas->getDescriptorImageInfo(fontAtlas->getSampler());
    core::DescriptorWriter(*userInterfaceDescLayout_, *context.descriptorPool)
        .writeImage(0, &imageInfo)
        .build(userInterfaceDescSet_);

    // Build the user interface pipeline
    userInterfacePipeline_ =
        core::GraphicsPipelineBuilder(*context.device)
            .addPushConstantRange(vk::PushConstantRange{.stageFlags = vk::ShaderStageFlagBits::eVertex,
                .offset = 0,
                .size = sizeof(UserInterfacePushConstants)})
            .addDescriptorSetLayout(userInterfaceDescLayout_)
            .setVertexShader(filesystem::readFile("../../spv/user_interface.vert.spv"), "main")
            .setFragmentShader(filesystem::readFile("../../spv/user_interface.frag.spv"), "main")
            .setDepthTest(false)
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addBlendAttachmentState(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                         vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                true)
            .setVertexInputAttributeDescriptions(UiVertex::getInputAttributeDescriptions())
            .setVertexInputBindingDescriptions(UiVertex::getInputBindingDescriptions())
            .addColorAttachmentFormat(vk::Format::eR8G8B8A8Unorm)
            .build();

    // Create the user interface vertex buffer
    userInterfaceVertexBuffer_ = context.resourceManager->createResource<core::Buffer>(core::BufferDesc{
        .type = core::BufferType::HostVisible,
        .usage = core::BufferUsage::VertexBuffer | core::BufferUsage::TransferDst,
        .instanceSize = sizeof(UiVertex),
        .instanceCount = 10000  // for now
    });
    context.resourceManager->getResource<core::Buffer>(userInterfaceVertexBuffer_)->map();
}

void hammock::renderer::Renderer::drawFrame(
    core::ResourceHandle target, RenderSnapshot& snap, core::Semaphore& signal) {
    if (!target.isValid() || target.getType() != core::ResourceType::Image) {
        throw std::runtime_error("Invalid rendering target");
    }

    auto& commandBuffer = *commandBuffers_[currentFrameIdx_];
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
    attachment.clearValue.setColor(
        vk::ClearColorValue(std::array<float, 4>{snap.x / 255.f, snap.x / 255.f, snap.x / 255.f, 1.0f}));

    // core::Logger::debug("Packet x: %d", packet.x);

    vk::RenderingInfo renderInfo{};
    renderInfo.renderArea = vk::Rect2D{{0, 0}, {image->getExtent().width, image->getExtent().height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attachment;

    commandBuffer.getCommandBuffer().beginRendering(&renderInfo);

    // Render the ui
    // Update the ui vert buffer
    core::Buffer* uiVertBuffer = ctx_.resourceManager->getResource<core::Buffer>(userInterfaceVertexBuffer_);
    uiVertBuffer->writeToBuffer(
        snap.uiDraws.vertices.data(), snap.uiDraws.vertices.size() * sizeof(UiVertex));
    // Bind the ui vert buffer
    vk::DeviceSize offsets[1]{0};
    commandBuffer.getCommandBuffer().bindVertexBuffers(0, 1, uiVertBuffer->getBufferPtr(), offsets);

    // Bind the ui pipeline
    userInterfacePipeline_->bind(commandBuffer);

    // Bind the descriptor set
    commandBuffer.getCommandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        userInterfacePipeline_->getPipelineLayout(),
        0,
        userInterfaceDescSet_,
        nullptr);

    // Scissors and viewport
    userInterfacePipeline_->setViewport(
        commandBuffer, 0, 0, image->getExtent().width, image->getExtent().height, 0, 1);
    userInterfacePipeline_->setScissor(commandBuffer,
        {.x = 0, .y = 0},
        {.width = image->getExtent().width, .height = image->getExtent().height});

    // Push constants
    UserInterfacePushConstants push{
        .screenSize = {static_cast<float>(image->getExtent().width), static_cast<float>(image->getExtent().height)}};
    userInterfacePipeline_->pushConstants(commandBuffer, vk::ShaderStageFlagBits::eVertex, 0, sizeof(UserInterfacePushConstants), &push);

    // Draw the ui
    commandBuffer.getCommandBuffer().draw(snap.uiDraws.vertices.size(), 1, 0, 0);

    commandBuffer.getCommandBuffer().endRendering();

    commandBuffer.submit();

    // Update the frame index
    nextFrameIdx();
}

void hammock::renderer::Renderer::nextFrameIdx() {
    currentFrameIdx_ = (currentFrameIdx_ + 1) % core::SwapChain::MAX_FRAMES_IN_FLIGHT;
}
