#include "render_graph_executor.hpp"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "command_buffer.hpp"
#include "device.hpp"
#include "swapchain.hpp"

hammock::graph::ExecutionPlan hammock::graph::RenderGraphExecutionPlanner::plan(CompiledRenderGraph& graph) {
    std::vector<CommandBufferBatch> batches;

    std::unordered_map<uint32_t, uint32_t> passToBatch;
    std::unordered_map<core::CommandQueueFamily, uint32_t> openBatch;
    std::unordered_map<core::CommandQueueFamily, uint64_t> nextSignalValue;

    // Assigns a signal value to a batch but does NOT touch openBatch.
    // Safe to call while iterating openBatch.
    auto sealBatch = [&](uint32_t idx) {
        auto& b = batches[idx];
        if (b.signalValue != 0) return;  // already sealed
        b.signalValue = ++nextSignalValue[b.family];
    };

    // Seals a batch AND removes it from openBatch.
    // Do NOT call while iterating openBatch.
    auto closeBatch = [&](uint32_t idx) {
        sealBatch(idx);
        openBatch.erase(batches[idx].family);
    };

    auto openNewBatch =
        [&](core::CommandQueueFamily family, std::vector<CommandBufferBatch::SemaphoreWait> waits)
        -> uint32_t {
        const uint32_t idx = static_cast<uint32_t>(batches.size());
        batches.push_back({family, {}, 0, std::move(waits)});
        openBatch[family] = idx;
        return idx;
    };

    // Main loop
    for (const auto& level : graph.levels) {
        for (const uint32_t passIdx : level) {
            const auto& pass = graph.passes[passIdx];

            std::unordered_set<uint32_t> foreignBatches;

            for (const uint32_t depIdx : pass.dependencies) {
                const auto it = passToBatch.find(depIdx);
                if (it == passToBatch.end()) continue;

                const uint32_t depBatchIdx = it->second;
                if (graph.passes[depIdx].family != pass.family) foreignBatches.insert(depBatchIdx);
            }

            uint32_t targetBatch;

            if (!foreignBatches.empty()) {
                // Seal every foreign batch — they will signal their timelines.
                for (const uint32_t fi : foreignBatches) closeBatch(fi);

                // Seal the current same-family batch (if any). Once we open a new
                // submission, implicit queue ordering no longer applies.
                std::vector<CommandBufferBatch::SemaphoreWait> allWaits;

                const auto ownIt = openBatch.find(pass.family);
                if (ownIt != openBatch.end()) {
                    const uint32_t ownIdx = ownIt->second;
                    closeBatch(ownIdx);  // erases from openBatch
                    allWaits.push_back({ownIdx, batches[ownIdx].signalValue});
                }

                for (const uint32_t fi : foreignBatches) allWaits.push_back({fi, batches[fi].signalValue});

                targetBatch = openNewBatch(pass.family, std::move(allWaits));
            } else {
                // Same-queue (or no) dependency — append to the open batch,
                // or start a fresh one if none exists yet.
                const auto it = openBatch.find(pass.family);
                targetBatch = (it != openBatch.end()) ? it->second : openNewBatch(pass.family, {});
            }

            batches[targetBatch].passes.push_back(passIdx);
            passToBatch[passIdx] = targetBatch;
        }
    }

    for (const auto& [family, idx] : openBatch) sealBatch(idx);
    openBatch.clear();

    return ExecutionPlan{.batches = batches};
}


hammock::graph::RenderGraphExecutor::RenderGraphExecutor(core::Device& device, ExecutionPlan& plan, CompiledRenderGraph& graph)
    : device_(device), plan_(plan), graph_(graph){
    // Get number of processors
    auto concurrency = std::thread::hardware_concurrency();
    // Set thread count to number of processors
    threadPool_.setThreadCount(concurrency);
    // Create command pools
    graphicsPool_ = commandPools_.create(device_, core::CommandQueueFamily::Graphics);
    computePool_ = commandPools_.create(device_, core::CommandQueueFamily::Compute);
    transferPool_ = commandPools_.create(device_, core::CommandQueueFamily::Transfer);
    // Allocate command buffers
    perFrameResources_.resize(core::SwapChain::MAX_FRAMES_IN_FLIGHT);
    core::SwapChain::forEachFrameInFlight([this](uint32_t frameIdx){
        for(auto& batch: plan_.batches){
            core::Handle<core::CommandBuffer> handle;
            if(batch.family == core::CommandQueueFamily::Graphics){
                handle = commandBuffers_.create(commandPools_.get(graphicsPool_));
            } else if (batch.family == core::CommandQueueFamily::Compute){
                handle = commandBuffers_.create(commandPools_.get(computePool_));
            } else if (batch.family == core::CommandQueueFamily::Transfer){
                handle = commandBuffers_.create(commandPools_.get(transferPool_));
            } else throw std::runtime_error("invalid batch family");

            perFrameResources_[frameIdx].commandBuffers.push_back(handle);
        }
    });
}

void hammock::graph::RenderGraphExecutor::execute() {
    for(int i = 0; i < plan_.batches.size(); i++){
        auto& batch = plan_.batches[i];
        auto commandBuffer = commandBuffers_.ref(perFrameResources_[frameIdx].commandBuffers[i]);



        for(auto& p: batch.passes){
            auto& pass = graph_.passes[p];


        }
    }
}

void hammock::graph::RenderGraphExecutor::nextFrameIdx() {
    frameIdx = (frameIdx + 1) % core::SwapChain::MAX_FRAMES_IN_FLIGHT;
}
