#include "renderer.hpp"

#include <functional>
#include <memory>
#include <span>

#include "buffer.hpp"
#include "core/swapchain.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "pipeline.hpp"
#include "resource_manager.hpp"
#include "ui/font.hpp"
#include "ui/ui.hpp"
#include "utils/filesystem.hpp"
#include "vertex.hpp"
#include "vulkan/vulkan.hpp"

hammock::renderer::Renderer::Renderer(hammock::renderer::Renderer::SurfaceFactory surfaceFactory, hammock::renderer::Renderer::SurfaceDestructor surfaceDestructor)
    : instance_{},
      surface_(surfaceFactory(instance_)),
      device_(instance_, surface_),
      surfaceDestructor_(std::move(surfaceDestructor)) {
    // create the descriptor pool
    descriptorPoolHandle_ = descriptorPools_.create(device_,
        10000,
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        std::vector<vk::DescriptorPoolSize>{
            {vk::DescriptorType::eSampler, 1000},
            {vk::DescriptorType::eCombinedImageSampler, 1000},
            {vk::DescriptorType::eSampledImage, 1000},
            {vk::DescriptorType::eStorageImage, 1000},
            {vk::DescriptorType::eUniformTexelBuffer, 1000},
            {vk::DescriptorType::eStorageTexelBuffer, 1000},
            {vk::DescriptorType::eUniformBuffer, 1000},
            {vk::DescriptorType::eStorageBuffer, 1000},
            {vk::DescriptorType::eUniformBufferDynamic, 1000},
            {vk::DescriptorType::eStorageBufferDynamic, 1000},
            {vk::DescriptorType::eInputAttachment, 1000},
        });

    core::SwapChain::forEachFrameInFlight([this](int i) {
        auto commandBuffer = commandBuffers_.create(device_, core::CommandQueueFamily::Graphics);
        cmds.push_back(commandBuffer);
    });

    // Create font atlas GPU resource
    ui::FontAtlas& atlas = ui::Ui::instance().getFontAtlas();
    fontAtlasHandle = images_.create(device_,
        core::ImageDesc{.width = ui::ATLAS_WIDTH,
            .height = ui::ATLAS_HEIGHT,
            .channels = 1,
            .format = core::ImageFormat::R8Uint,
            .usage = core::ImageUsage::Sampled | core::ImageUsage::TransferDst,
            .type = core::ImageType::Type2D});
    auto fontAtlas = images_.ref(fontAtlasHandle);
    fontAtlas->createSampler();
    auto atlasStagingBufferHandle = buffers_.create(device_,
        core::BufferDesc{.type = core::BufferType::HostVisible,
            .usage = core::BufferUsage::TransferSrc | core::BufferUsage::TransferDst,
            .instanceSize = sizeof(uint8_t),
            .instanceCount = ui::ATLAS_WIDTH * ui::ATLAS_HEIGHT});
    auto atlasStagingBuffer = buffers_.ref(atlasStagingBufferHandle);
    atlasStagingBuffer->map();
    atlasStagingBuffer->writeToBuffer(atlas.bitmap.data());

    auto prepCmdHandle = commandBuffers_.create(device_, core::CommandQueueFamily::Graphics);
    auto prepCmd = commandBuffers_.ref(prepCmdHandle);

    prepCmd->begin();
    prepCmd->imagePipelineBarrier(fontAtlas,
        vk::PipelineStageFlagBits2::eNone,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);
    prepCmd->copyBufferToImage(atlasStagingBuffer, fontAtlas);
    prepCmd->imagePipelineBarrier(fontAtlas,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal);
    prepCmd->submit();

    device_.waitIdle();
    atlasStagingBuffer->unmap();
    buffers_.destroy(atlasStagingBufferHandle);

    // Create font atlas descriptor
    userInterfaceDescLayout_ =
        core::DescriptorSetLayoutBuilder(device_)
            .addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
            .build();
    auto imageInfo = fontAtlas->getDescriptorImageInfo(fontAtlas->getSampler());
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    core::DescriptorWriter(*userInterfaceDescLayout_, descriptorPools_.ref(descriptorPoolHandle_).get())
        .writeImage(0, &imageInfo)
        .build(userInterfaceDescSet_);

    // Build the user interface pipeline
    userInterfacePipeline_ = pipelines_.create(core::PipelineBuilder(device_)
            .setVertexShader(filesystem::readFile("../../spv/user_interface.vert.spv"), "main")
            .setFragmentShader(filesystem::readFile("../../spv/user_interface.frag.spv"), "main")
            .addPushConstantRange(vk::PushConstantRange{.stageFlags = vk::ShaderStageFlagBits::eVertex,
                .offset = 0,
                .size = sizeof(UserInterfacePushConstants)})
            .addDescriptorSetLayout(userInterfaceDescLayout_)
            .setDepthTest(false)
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addBlendAttachmentState(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                         vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                true)
            .setVertexInputAttributeDescriptions(UiVertex::getInputAttributeDescriptions())
            .setVertexInputBindingDescriptions(UiVertex::getInputBindingDescriptions())
            .addColorAttachmentFormat(vk::Format::eR8G8B8A8Unorm)
            .buildCreateInfo());

    // Create the user interface vertex buffer
    userInterfaceVertexBuffer_ = buffers_.create(device_,
        core::BufferDesc{
            .type = core::BufferType::HostVisible,
            .usage = core::BufferUsage::VertexBuffer | core::BufferUsage::TransferDst,
            .instanceSize = sizeof(UiVertex),
            .instanceCount = 10000  // for now
        });
    buffers_.ref(userInterfaceVertexBuffer_)->map();  // make it constantly mapped
}

hammock::renderer::Renderer::~Renderer() { surfaceDestructor_(instance_, surface_); }

void hammock::renderer::Renderer::drawFrame(
    core::ResourceRef<core::Image> target, RenderSnapshot& snap, core::ResourceRef<core::Semaphore> signal) {
    // Get the command buffer reference from the manager
    auto commandBuffer = commandBuffers_.ref(cmds[currentFrameIdx_]);

    commandBuffer->addSignalSemaphore(signal);
    commandBuffer->begin();

    // Attachment needs to be in color attachment optimal layout
    commandBuffer->imagePipelineBarrier(target,
        vk::PipelineStageFlagBits2::eNone,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);

    auto attachment = target->getRenderingAttachmentInfo();
    attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;

    auto targets = std::array<core::ResourceRef<core::Image>, 1>{target};
    auto layouts = std::array<vk::ImageLayout, 1>{vk::ImageLayout::eColorAttachmentOptimal};

    commandBuffer->beginRendering(
        {{0, 0}, {target->getExtent().width, target->getExtent().height}}, targets, layouts);

    // Render the ui
    // Update the ui vert buffer
    auto uiVertBuffer = buffers_.ref(userInterfaceVertexBuffer_);
    uiVertBuffer->writeToBuffer(
        snap.uiDraws.vertices.data(), snap.uiDraws.vertices.size() * sizeof(UiVertex));
    // Bind the ui vert buffer
    vk::DeviceSize offsets[1]{0};
    commandBuffer->getCommandBuffer().bindVertexBuffers(0, 1, uiVertBuffer->getBufferPtr(), offsets);
    commandBuffer->bindVertexBuffers(std::span(&uiVertBuffer, 1), offsets);

    // Bind the ui pipeline
    commandBuffer->bindPipeline(pipelines_.ref(userInterfacePipeline_));

    // Bind the descriptor set
    commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        pipelines_.ref(userInterfacePipeline_),
        std::span(&userInterfaceDescSet_, 1));

    // Scissors and viewport
    commandBuffer->setViewport({0,
        0,
        static_cast<float>(target->getExtent().width),
        static_cast<float>(target->getExtent().height),
        0.f,
        1.f});
    commandBuffer->setScissor({0, 0, target->getExtent().width, target->getExtent().height});

    // Push constants
    UserInterfacePushConstants push{.screenSize = {static_cast<float>(target->getExtent().width),
                                        static_cast<float>(target->getExtent().height)}};
    commandBuffer->pushConstants(pipelines_.ref(userInterfacePipeline_),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(UserInterfacePushConstants),
        &push);

    // Draw the ui
    commandBuffer->getCommandBuffer().draw(snap.uiDraws.vertices.size(), 1, 0, 0);

    commandBuffer->endRendering();

    commandBuffer->submit();

    // Update the frame index
    nextFrameIdx();
}

void hammock::renderer::Renderer::nextFrameIdx() {
    currentFrameIdx_ = (currentFrameIdx_ + 1) % core::SwapChain::MAX_FRAMES_IN_FLIGHT;
}
void hammock::renderer::Renderer::waitIdle() { device_.waitIdle(); }
