#include "frame_graph.hpp"

#include <stdexcept>

#include "command_buffer.hpp"
#include "image.hpp"
#include "resource_manager.hpp"

hammock::renderer::FrameGraph::FrameGraph(FrameGraphDesc&& desc) : desc_{std::move(desc)} {}

void hammock::renderer::FrameGraph::execute() {
    // Reset command pool to enable recording over alredy allocated command buffers
    desc_.commandCache.reset();

    // For each batch determine its family and allocate command buffer
    for (auto& batch : desc_.batches) {
        // Make sure only one family is present
        if ((!batch.graphics.empty() + !batch.compute.empty() + !batch.transfer.empty()) != 1) {
            // more than one is non-empty -> this is error, early exit
            throw std::runtime_error("more than one type of passes in a batch is not supported");
        }

        // Get the primary cmd buffer reference
        core::ResourceRef<core::CommandBuffer> primaryCmd = desc_.commandCache.getPrimaryCommandBuffer();

        // Begin command buffer
        primaryCmd->begin();

        if (batch.semaphore.has_value() && batch.wait.has_value()) {
            primaryCmd->addWaitSemaphore(batch.semaphore.value(), batch.waitStage, batch.wait);
        }

        if (batch.semaphore.has_value() && batch.singal.has_value()) {
            primaryCmd->addSignalSemaphore(batch.semaphore.value(), batch.singal);
        }

        // Record all passes in the batch
        if (!batch.graphics.empty()) {
            for (auto& pass : batch.graphics) {
                recordGraphicsPass(pass, primaryCmd);
            }
        } else if (!batch.compute.empty()) {
            for (auto& pass : batch.compute) {
                recordBarrier(pass.transitions, primaryCmd);
                pass.execute(primaryCmd);
            }
        } else if (!batch.transfer.empty()) {
            for (auto& pass : batch.transfer) {
                recordBarrier(pass.transitions, primaryCmd);
                pass.execute(primaryCmd);
            }
        }

        primaryCmd->submit();
    }
}

void hammock::renderer::FrameGraph::recordBarrier(
    PassTransitions& transitions, core::ResourceRef<core::CommandBuffer> cmd) {
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
    pass.execute(cmd);

    // Automatically end rendering
    if (beginRendering) {
        cmd->endRendering();
    }
}

void hammock::renderer::CommandBufferCache::init(
    core::Device& device, core::CommandQueueFamily family, uint32_t numThreads) {
    commandPools_.clear();  // Clear first
    commandBuffers_.clear();

    // Create the main pool and primary command buffer
    mainPoolHndl_ = commandPools_.create(device, family);
    primaryBufferHndl_ =
        commandBuffers_.create(commandPools_.get(mainPoolHndl_), core::CommandBufferLevel::Primary);

    // Create per thread secondary command buffers
    threadCaches_.resize(numThreads);
    for (uint32_t i = 0; i < numThreads; i++) {
        threadCaches_[i].secondaryBufferHdnl =
            commandBuffers_.create(commandPools_.get(mainPoolHndl_), core::CommandBufferLevel::Secondary);
    }
}

void hammock::renderer::CommandBufferCache::reset() { commandPools_.get(mainPoolHndl_).reset(); }

hammock::core::ResourceRef<hammock::core::CommandBuffer>
hammock::renderer::CommandBufferCache::getPrimaryCommandBuffer() {
    return commandBuffers_.ref(primaryBufferHndl_);
}

hammock::core::ResourceRef<hammock::core::CommandBuffer>
hammock::renderer::CommandBufferCache::getSecondaryCommandBuffer(uint32_t threadIndex) {
    if (threadIndex >= threadCaches_.size()) throw std::out_of_range("thread index out of range");
    return commandBuffers_.ref(threadCaches_[threadIndex].secondaryBufferHdnl);
}
