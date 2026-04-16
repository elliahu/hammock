#include "frame_graph.hpp"

#include <stdexcept>
#include <variant>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"

hammock::renderer::FrameGraph::FrameGraph(FrameGraphDesc&& desc, CommandCache& commandCache)
    : desc_{std::move(desc)}, commandCache_(commandCache) {}

void hammock::renderer::FrameGraph::execute(uint32_t frameIndex) {
    // Reset pools
    commandCache_.resetPools(frameIndex);

    // For each batch determine its family and allocate command buffer
    for (auto& pass : desc_.passes) {
        // Determin the pass family
        core::CommandQueueFamily batchFamily = core::CommandQueueFamily::Ignored;
        if (std::holds_alternative<GraphicsPassDesc>(pass))
            batchFamily = core::CommandQueueFamily::Graphics;
        else if (std::holds_alternative<ComputePassDesc>(pass))
            batchFamily = core::CommandQueueFamily::Compute;
        else if (std::holds_alternative<TransferPassDesc>(pass))
            batchFamily = core::CommandQueueFamily::Transfer;

        // Sanity check
        if (batchFamily == core::CommandQueueFamily::Ignored) {
            throw std::runtime_error("failed to determine pass family");
        }

        // Unsupported transfer
        // TODO
        if (batchFamily == core::CommandQueueFamily::Transfer) {
            throw std::runtime_error("transfer passes not yet supported");
        }

        // Get the primary cmd buffer reference
        core::ResourceRef<core::CommandBuffer> commandBuffer =
            commandCache_.getCommandBuffer(batchFamily, core::CommandBufferLevel::Primary, frameIndex, 0);

        // Begin command buffer
        commandBuffer->begin();

        // Add sync primitives and record
        std::visit(
            [&commandBuffer, this](auto& anyPass) {
                // Add wait semaphore if present
                if (anyPass.sync.semaphore.has_value() && anyPass.sync.wait.has_value()) {
                    commandBuffer->addWaitSemaphore(
                        anyPass.sync.semaphore.value(), anyPass.sync.waitStage, anyPass.sync.wait);
                }

                // Add signal semaphore if present
                if (anyPass.sync.semaphore.has_value() && anyPass.sync.signal.has_value()) {
                    commandBuffer->addSignalSemaphore(anyPass.sync.semaphore.value(), anyPass.sync.signal);
                }

                // Record
                using PassType = std::decay_t<decltype(anyPass)>;
                if constexpr (std::is_same_v<PassType, GraphicsPassDesc>) {
                    recordGraphicsPass(anyPass, commandBuffer);
                } else if constexpr (std::is_same_v<PassType, ComputePassDesc>) {
                    recordComputePass(anyPass, commandBuffer);
                }
            },
            pass);

        // Submit the command buffer
        commandBuffer->submit();
    }
}

void hammock::renderer::FrameGraph::recordBarrier(
    PassTransitionsInfo& transitions, core::ResourceRef<core::CommandBuffer> cmd) {
    if (!transitions.images.empty() || !transitions.buffers.empty()) {
        std::vector<core::ImageMemoryBarrier> imageBarriers{};
        std::vector<core::BufferMemoryBarrier> bufferBarriers{};

        for (auto& imageTransition : transitions.images) {
            imageBarriers.push_back(imageTransition);
        }

        for (auto& bufferTransition : transitions.buffers) {
            bufferBarriers.push_back(bufferTransition);
        }

        cmd->pipelineBarrier(imageBarriers, bufferBarriers);
    }
}

void hammock::renderer::FrameGraph::recordGraphicsPass(
    GraphicsPassDesc& pass, core::ResourceRef<core::CommandBuffer> cmd) {
    // Barriers first
    recordBarrier(pass.transitions, cmd);

    bool beginRendering =
        !pass.rendering.colorAttachments.empty() || pass.rendering.depthAttachment.has_value();
    bool setViewport = pass.rendering.viewport.has_value();
    bool setScissor = pass.rendering.scissors.has_value();
    bool bindPipeline = pass.rendering.pipeline.has_value();
    bool bindDescriptors = !pass.data.descriptors.empty();
    bool pushConstants = pass.data.constants.data != nullptr && pass.data.constants.size >= 0;

    // Automatically begin rendering
    if (beginRendering) {
        // If render are is not set, color first attachment is used
        auto renderArea = pass.rendering.area.value_or(
            pass.rendering.colorAttachments.empty() ? pass.rendering.depthAttachment.value()->createScissor()
                                                    : pass.rendering.colorAttachments[0]->createScissor());
        cmd->beginRendering(renderArea,
            pass.rendering.colorAttachments,
            pass.rendering.depthAttachment,
            pass.rendering.stencilAttachment);
    }

    // Bind pipeline
    if (bindPipeline) {
        cmd->bindPipeline(pass.rendering.pipeline.value());
    }

    // Set viewport
    if (setViewport) {
        cmd->setViewport(pass.rendering.viewport.value());
    }

    // Set scissor
    if (setScissor) {
        cmd->setScissor(pass.rendering.scissors.value());
    }

    // Bind descriptors
    if (bindDescriptors) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot bind descriptors");

        cmd->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pass.rendering.pipeline.value(), pass.data.descriptors);
    }

    // Push constants
    if (pushConstants) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot push constants");

        cmd->pushConstants(pass.rendering.pipeline.value(),
            pass.data.constants.stages,
            0,
            pass.data.constants.size,
            pass.data.constants.data);
    }

    // Execute custom code
    if (pass.execute) {
        pass.execute(cmd);
    }

    // Automatically end rendering
    if (beginRendering) {
        cmd->endRendering();
    }
}

void hammock::renderer::FrameGraph::recordComputePass(
    ComputePassDesc& pass, core::ResourceRef<core::CommandBuffer> cmd) {
    // Barriers first
    recordBarrier(pass.transitions, cmd);

    bool dispatch = pass.dispatch.sizes.x > 0 && pass.dispatch.sizes.y > 0 && pass.dispatch.sizes.z > 0;
    bool bindPipeline = pass.dispatch.pipeline.has_value();
    bool bindDescriptors = !pass.data.descriptors.empty();
    bool pushConstants = pass.data.constants.data != nullptr && pass.data.constants.size >= 0;

    // Bind pipeline
    if (bindPipeline) {
        cmd->bindPipeline(pass.dispatch.pipeline.value());
    }

    // Bind descriptors
    if (bindDescriptors) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot bind descriptors");

        cmd->bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, pass.dispatch.pipeline.value(), pass.data.descriptors);
    }

    // Push constants
    if (pushConstants) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot push constants");

        cmd->pushConstants(pass.dispatch.pipeline.value(),
            pass.data.constants.stages,
            0,
            pass.data.constants.size,
            pass.data.constants.data);
    }

    // Dispatch
    if (dispatch) {
        cmd->dispatch(pass.dispatch.sizes.x, pass.dispatch.sizes.y, pass.dispatch.sizes.z);
    }

    // Execute custom code
    if (pass.execute) {
        pass.execute(cmd);
    }
}

hammock::core::Handle<hammock::renderer::FrameGraphBuilder::FrameGraphPassWrapper>
hammock::renderer::FrameGraphBuilder::createGraphicsPass(GraphicsPassDesc&& graphicsDesc) {
    auto handle = passes_.create(std::move(graphicsDesc));
    handles_.push_back(handle);
    return handle;
}

hammock::core::Handle<hammock::renderer::FrameGraphBuilder::FrameGraphPassWrapper>
hammock::renderer::FrameGraphBuilder::createComputePass(ComputePassDesc&& computeDesc) {
    auto handle = passes_.create(std::move(computeDesc));
    handles_.push_back(handle);
    return handle;
}

hammock::renderer::FrameGraphDesc hammock::renderer::FrameGraphBuilder::build() {
    FrameGraphDesc desc{};
    for(auto& handle : handles_){
        core::ResourceRef<FrameGraphPassWrapper> pass = passes_.ref(handle);
        desc.passes.push_back(pass->desc);
    }
    return desc;
}
