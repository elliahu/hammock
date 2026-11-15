#include "frame_graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <variant>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "device.hpp"
#include "image.hpp"
#include "pass_recorder.hpp"
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
    PassRecorder recorder;

    // For each batch determine its family and allocate command buffer
    for (auto& pass : desc_.passes) {
        // Determine the pass family
        // And get correct timeline semaphore for the queue
        core::CommandQueueFamily queue = pass.family;
        core::ResourceRef<core::Semaphore> familyTimeline;
        if (pass.family == core::CommandQueueFamily::Graphics) {
            queue = core::CommandQueueFamily::Graphics;
            familyTimeline = timeline_.graphics;
        } else if (pass.family == core::CommandQueueFamily::Compute) {
            queue = core::CommandQueueFamily::Compute;
            familyTimeline = timeline_.compute;
        } else if (pass.family == core::CommandQueueFamily::Transfer) {
            queue = core::CommandQueueFamily::Transfer;
            familyTimeline = timeline_.transfer;
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
            commandBuffer->addSignalSemaphore(familyTimeline, signal.value);
        }

        recordBarrier(pass.transitions, commandBuffer);
        recorder.record(pass, commandBuffer);

        // Submit the command buffer
        commandBuffer->submit();
    }
}

void hammock::renderer::FrameGraph::recordBarrier(
    PassTransitionsInfo& transitions, core::ResourceRef<core::CommandBuffer> commandBuffer) {
    if (!transitions.images.empty() || !transitions.buffers.empty()) {
        std::vector<core::ImageMemoryBarrier> imageBarriers;
        std::vector<core::BufferMemoryBarrier> bufferBarriers;

        imageBarriers.insert(imageBarriers.end(), transitions.images.begin(), transitions.images.end());
        bufferBarriers.insert(bufferBarriers.end(), transitions.buffers.begin(), transitions.buffers.end());
        commandBuffer->pipelineBarrier(imageBarriers, bufferBarriers);
    }
}
