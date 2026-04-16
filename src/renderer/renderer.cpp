#include "renderer.hpp"

#include <functional>
#include <span>
#include <utility>

#include "buffer.hpp"
#include "command_buffer.hpp"
#include "core/swapchain.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "frame_graph.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "pipeline.hpp"
#include "resource_manager.hpp"
#include "ui/font.hpp"
#include "ui/ui.hpp"
#include "utils/filesystem.hpp"
#include "vertex.hpp"
#include "vulkan/vulkan.hpp"

hammock::renderer::Renderer::Renderer(hammock::renderer::Renderer::SurfaceFactory surfaceFactory,
    hammock::renderer::Renderer::SurfaceDestructor surfaceDestructor)
    : instance_{},
      surface_(surfaceFactory(instance_)),
      device_(instance_, surface_),
      surfaceDestructor_(std::move(surfaceDestructor)),
      commandBufferProvider_(device_, core::SwapChain::MAX_FRAMES_IN_FLIGHT, 1) {
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

    // Create thread pool
    threadPool_.setThreadCount(std::thread::hardware_concurrency());

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

    prepPoolHandle_ = commandPools_.create(device_, core::CommandQueueFamily::Graphics);
    auto prepCmdHandle = commandBuffers_.create(commandPools_.get(prepPoolHandle_));
    auto prepCmd = commandBuffers_.ref(prepCmdHandle);

    prepCmd->begin();
    std::array<core::ImageMemoryBarrier, 1> atlasNoneToTransfer{
        {fontAtlas, core::ResourceState::Undefined, core::ResourceState::TransferDst}};
    prepCmd->pipelineBarrier(atlasNoneToTransfer, {});
    prepCmd->copyBufferToImage(atlasStagingBuffer, fontAtlas);
    std::array<core::ImageMemoryBarrier, 1> atlasTransferToShaderRead{
        {fontAtlas, core::ResourceState::TransferDst, core::ResourceState::ShaderRead}};
    prepCmd->pipelineBarrier(atlasTransferToShaderRead, {});
    prepCmd->submit();

    device_.waitIdle();
    atlasStagingBuffer->unmap();
    buffers_.destroy(atlasStagingBufferHandle);
    commandBuffers_.destroy(prepCmdHandle);  // Destroy the prep command buffer

    // Create font atlas descriptor
    auto bindings = std::vector<vk::DescriptorSetLayoutBinding>{{.binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment}};
    auto flags = std::vector<vk::DescriptorBindingFlags>{};
    userInterfaceDescLayoutHandle_ = descriptorSetLayouts_.create(device_, bindings, flags);
    auto imageInfo = fontAtlas->getDescriptorImageInfo(fontAtlas->getSampler());
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    core::DescriptorWriter(*descriptorSetLayouts_.ref(userInterfaceDescLayoutHandle_),
        descriptorPools_.ref(descriptorPoolHandle_).get())
        .writeImage(0, &imageInfo)
        .build(userInterfaceDescSet_);

    // Build the user interface pipeline
    userInterfacePipelineHandle_ = pipelines_.create(core::PipelineBuilder(device_)
            .asGraphicsPipeline()
            .setVertexShader(filesystem::readFile("../../spv/user_interface.vert.spv"), "main")
            .setFragmentShader(filesystem::readFile("../../spv/user_interface.frag.spv"), "main")
            .addPushConstantRange(vk::PushConstantRange{.stageFlags = vk::ShaderStageFlagBits::eVertex,
                .offset = 0,
                .size = sizeof(UserInterfacePushConstants)})
            .addDescriptorSetLayout(descriptorSetLayouts_.ref(userInterfaceDescLayoutHandle_))
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

    // Create compute descriptors
    bindings = std::vector<vk::DescriptorSetLayoutBinding>{{.binding = 0,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute}};
    computeLayoutHandle_ = descriptorSetLayouts_.create(device_, bindings, flags);

    // Create the compute pipeline
    computePipelineHandle_ = pipelines_.create(core::PipelineBuilder(device_)
            .asComputePipeline()
            .setComputeShader(filesystem::readFile("../../spv/fullscreen_color.comp.spv"))
            .addPushConstantRange(vk::PushConstantRange{.stageFlags = vk::ShaderStageFlagBits::eCompute,
                .offset = 0,
                .size = sizeof(ComputePushConstants)})
            .addDescriptorSetLayout(descriptorSetLayouts_.ref(computeLayoutHandle_))
            .buildCreateInfo());

    // Create sync sempahore
    timelineSemaphoreHandle_ = semaphores_.create(device_, 0);
}

hammock::renderer::Renderer::~Renderer() {
    if (surfaceDestructor_) {
        surfaceDestructor_(instance_, surface_);
    }
}

void hammock::renderer::Renderer::drawFrame(
    core::ResourceRef<core::Image> target, RenderSnapshot& snap, core::ResourceRef<core::Semaphore> signal) {
    UserInterfacePushConstants uiPush{.screenSize = {static_cast<float>(target->getExtent2D().x),
                                          static_cast<float>(target->getExtent2D().y)}};
    ComputePushConstants compPush{.color = {1.f, 0.f, 0.f}};
    constexpr uint32_t groupSize = 16u;

    // Create compute descriptor set
    if (!target->getSampler()) {
        target->createSampler();
    }
    auto targetDescriptorInfo = target->getDescriptorImageInfo();
    targetDescriptorInfo.imageLayout = vk::ImageLayout::eGeneral;
    auto writer = core::DescriptorWriter(
        *descriptorSetLayouts_.ref(computeLayoutHandle_), descriptorPools_.ref(descriptorPoolHandle_).get())
                      .writeImage(0, &targetDescriptorInfo);
    if (computeDescriptorSets_[currentFrameIdx_])
        writer.overwrite(computeDescriptorSets_[currentFrameIdx_]);
    else
        writer.build(computeDescriptorSets_[currentFrameIdx_]);

    ComputePassDesc computeDesc{
        .name = "Compute pass",
        .transitions =
            {
                .images = {{target, core::ResourceState::Undefined, core::ResourceState::StorageImage}},
            },
        .dispatch =
            {
                .sizes =
                    {
                        .x = (target->getExtent2D().x + groupSize - 1) / groupSize,
                        .y = (target->getExtent2D().y + groupSize - 1) / groupSize,
                        .z = 1,
                    },
                .pipeline = pipelines_.ref(computePipelineHandle_),
            },
        .data =
            {
                .descriptors = {computeDescriptorSets_[currentFrameIdx_]},
                .constants =
                    {
                        .stages = core::ShaderStage::Compute,
                        .size = sizeof(ComputePushConstants),
                        .data = &compPush,
                    },
            },
            .sync =
                {
                    .semaphore = semaphores_.ref(timelineSemaphoreHandle_),
                    .signal = timelineValue,
                },
    };

    GraphicsPassDesc uiPassDesc{
        .name = "User Interface",
        .transitions =
            {
                .images = {{target, core::ResourceState::StorageImage, core::ResourceState::ColorAttachment}},
            },
        .rendering =
            {
                .colorAttachments = {target},
                .viewport = target->createViewport(),
                .scissors = target->createScissor(),
                .pipeline = pipelines_.ref(userInterfacePipelineHandle_),
            },
        .data =
            {
                .descriptors = {userInterfaceDescSet_},
                .constants =
                    {
                        .stages = core::ShaderStage::Vertex,
                        .size = sizeof(UserInterfacePushConstants),
                        .data = &uiPush,
                    },
            },
            .sync =
                {
                    .semaphore = semaphores_.ref(timelineSemaphoreHandle_),
                    .wait = timelineValue,
                    .waitStage = core::PipelineStage::AllCommands,
                },
        .execute =
            [&, this](core::ResourceRef<core::CommandBuffer> commandBuffer) {
                // Signal rendering finished
                commandBuffer->addSignalSemaphore(signal);

                // Render the ui
                // Update the ui vert buffer
                auto uiVertBuffer = buffers_.ref(userInterfaceVertexBuffer_);
                uiVertBuffer->writeToBuffer(
                    snap.uiDraws.vertices.data(), snap.uiDraws.vertices.size() * sizeof(UiVertex));
                // Bind the ui vert buffer
                vk::DeviceSize offsets[1]{0};
                commandBuffer->getCommandBuffer().bindVertexBuffers(
                    0, 1, uiVertBuffer->getBufferPtr(), offsets);
                commandBuffer->bindVertexBuffers(std::span(&uiVertBuffer, 1), offsets);

                // Draw the ui
                commandBuffer->getCommandBuffer().draw(snap.uiDraws.vertices.size(), 1, 0, 0);
            },
    };

    FrameGraph graph{{
        .passes = {std::move(computeDesc), std::move(uiPassDesc)},
        .commandCache = commandBufferProvider_,
    }};

    graph.execute(currentFrameIdx_);

    // Update the frame index
    nextFrameIdx();
}

void hammock::renderer::Renderer::nextFrameIdx() {
    currentFrameIdx_ = (currentFrameIdx_ + 1) % core::SwapChain::MAX_FRAMES_IN_FLIGHT;
    timelineValue++;
}
void hammock::renderer::Renderer::waitIdle() { device_.waitIdle(); }
