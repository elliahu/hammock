#include "frame_graph.hpp"

#include <stdexcept>

#include "command_buffer.hpp"
#include "resource_manager.hpp"

hammock::renderer::FrameGraph::FrameGraph(FrameGraphDesc&& desc) : desc_{std::move(desc)} {}

void hammock::renderer::FrameGraph::execute() {
    // Release stale handles and reset the managers internal vectors
    desc_.commandBufferManager.clear();

    // Reset command pools
    if (desc_.graphicsPool.has_value()) {
        desc_.graphicsPool.value()->reset();
    }

    if (desc_.computePool.has_value()) {
        desc_.computePool.value()->reset();
    }

    if (desc_.transferPool.has_value()) {
        desc_.transferPool.value()->reset();
    }

    // For each batch determine its family and allocate command buffer
    for (auto& batch : desc_.batches) {
        // Make sure only one family is present
        if ((!batch.graphics.empty() + !batch.compute.empty() + !batch.transfer.empty()) != 1) {
            // more than one is non-empty -> this is error, early exit
            throw std::runtime_error("more than one type of passes in a batch is not supported");
        }

        // Determine the batch family
        // Allocate primary command buffer for this batch
        core::Handle<core::CommandBuffer> primaryCmdHndl;
        uint32_t variant = 0;
        if (!batch.graphics.empty()) {
            if (desc_.graphicsPool.has_value()) {
                primaryCmdHndl = desc_.commandBufferManager.create(
                    desc_.graphicsPool.value().get(), core::CommandBufferLevel::Primary);
                variant = 0;
            } else {
                throw std::runtime_error(
                    "batch type determined to be graphics but no graphics pool was provided");
            }
        } else if (!batch.compute.empty()) {
            if (desc_.computePool.has_value()) {
                primaryCmdHndl = desc_.commandBufferManager.create(
                    desc_.computePool.value().get(), core::CommandBufferLevel::Primary);
                variant = 1;
            } else {
                throw std::runtime_error(
                    "batch type determined to be compute but no compute pool was provided");
            }
        } else if (!batch.transfer.empty()) {
            if (desc_.transferPool.has_value()) {
                primaryCmdHndl = desc_.commandBufferManager.create(
                    desc_.transferPool.value().get(), core::CommandBufferLevel::Primary);
                variant = 2;
            } else {
                throw std::runtime_error(
                    "batch type determined to be transfer but no transfer pool was provided");
            }
        }
        // Get the primary cmd buffer reference
        core::ResourceRef<core::CommandBuffer> primaryCmd = desc_.commandBufferManager.ref(primaryCmdHndl);

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
            if (batch.strategy == RecordingStrategy::Multithreaded) {
                for (auto& pass : batch.graphics) {
                    desc_.threadPool.submit([&, this](uint32_t t) {

                    });
                }

                desc_.threadPool.wait();

            } else {
                for (auto& pass : batch.graphics) {
                    recordBarrier(pass.transitions, primaryCmd);
                    pass.execute(primaryCmd);
                }
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
