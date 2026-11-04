#include "frame_graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <variant>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"

hammock::renderer::FrameGraph::FrameGraph(FrameGraphDescription&& desc, CommandCache& commandCache,
    core::ResourceRef<core::Semaphore> graphicsTimeline, core::ResourceRef<core::Semaphore> computeTimeline,
    core::ResourceRef<core::Semaphore> transferTimeline)
    : desc_{ std::move(desc) },
      commandCache_(commandCache),
      timeline_({ graphicsTimeline, computeTimeline, transferTimeline }) {}

void hammock::renderer::FrameGraph::execute(uint32_t frameIndex) {
    // Reset pools
    commandCache_.resetPools(frameIndex);

    // For each batch determine its family and allocate command buffer
    for (auto& pass : desc_.passes) {
        // Determine the pass family
        // And get correct timeline semaphore for the queue
        core::CommandQueueFamily queue = pass.family;
        core::ResourceRef<core::Semaphore> signalTimeline;
        if (pass.family == core::CommandQueueFamily::Graphics) {
            queue = core::CommandQueueFamily::Graphics;
            signalTimeline = timeline_.graphics;
        } else if (pass.family == core::CommandQueueFamily::Compute) {
            queue = core::CommandQueueFamily::Compute;
            signalTimeline = timeline_.compute;
        } else if (pass.family == core::CommandQueueFamily::Transfer) {
            queue = core::CommandQueueFamily::Transfer;
            signalTimeline = timeline_.transfer;
        }

        // Sanity check
        if (queue == core::CommandQueueFamily::Ignored) {
            throw std::runtime_error("failed to determine pass family");
        }

        // Unsupported transfer
        // TODO
        if (queue == core::CommandQueueFamily::Transfer) {
            throw std::runtime_error("transfer passes not yet supported");
        }

        // Get the primary cmd buffer reference
        core::ResourceRef<core::CommandBuffer> commandBuffer =
            commandCache_.getCommandBuffer(queue, core::CommandBufferLevel::Primary, frameIndex, 0);

        // Begin command buffer
        commandBuffer->begin();

        // Add sync primitives and record
        // Add wait semaphore if present
        for (auto& wait : pass.sync.waits) {
            core::ResourceRef<core::Semaphore> waitTimeline;

            // determine the queue on which we wait
            if (wait.queue == core::CommandQueueFamily::Graphics) {
                waitTimeline = timeline_.graphics;
            } else if (wait.queue == core::CommandQueueFamily::Compute) {
                waitTimeline = timeline_.compute;
            } else if (wait.queue == core::CommandQueueFamily::Transfer) {
                waitTimeline = timeline_.transfer;
            } else
                throw std::runtime_error("could not determine the wait queue");

            commandBuffer->addWaitSemaphore(waitTimeline, wait.stage, wait.value);
        }

        // Add signal semaphore
        for (auto& signal : pass.sync.signals) {
            commandBuffer->addSignalSemaphore(signalTimeline, signal.value);
        }

        // Record
        if (queue == core::CommandQueueFamily::Graphics) {
            recordGraphicsPass(pass, commandBuffer);
        } else if (queue == core::CommandQueueFamily::Compute) {
            recordComputePass(pass, commandBuffer);
        }

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
    PassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd) {
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
    PassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd) {
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
