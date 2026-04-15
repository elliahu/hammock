#include "command_buffer_provider.hpp"
#include "utilities.hpp"

hammock::renderer::CommandBufferProvider::CommandBufferProvider(
    core::Device& device, size_t frames, size_t threads)
    : device_(device), frameCount_(frames), threadCount_(threads) {
    auto indices = device.getPhysicalQueueFamilies();
    frames_.resize(frames);

    // f - frame index
    // q - queue (family) index
    // t - thread index

    for (int f = 0; f < frameCount_; f++) {
        // Allocate space for each queue type
        frames_[f].resize(3);  // There are three families

        for (int q = 0; q < 3; q++) {
            frames_[f][q].resize(threads);

            core::CommandQueueFamily family = static_cast<core::CommandQueueFamily>(
                static_cast<uint32_t>(core::CommandQueueFamily::Ignored) + q + 1);
            for (int t = 0; t < threads; t++) {
                frames_[f][q][t].pool = pools_.create(device_, family);
            }
        }
    }
}

hammock::core::ResourceRef<hammock::core::CommandBuffer>
hammock::renderer::CommandBufferProvider::getCommandBuffer(core::CommandQueueFamily family,
    core::CommandBufferLevel level, uint32_t frameIndex, uint32_t threadIndex) {
    auto& pool = frames_[frameIndex][static_cast<uint32_t>(family) - 1][threadIndex];
    return allocateOrReuse(pool, level);
}

void hammock::renderer::CommandBufferProvider::resetPools(uint32_t frameIndex) {
    for (int q = 0; q < 3; q++) {
        for (int t = 0; t < threadCount_; t++) {
            auto& pool = frames_[frameIndex][q][t];
            pools_.ref(pool.pool)->reset();
            pool.nextPrimary = 0;
            pool.nextSecondary = 0;
        }
    }
}

hammock::core::ResourceRef<hammock::core::CommandBuffer>
hammock::renderer::CommandBufferProvider::allocateOrReuse(
    LocalCommandBufferPool& pool, core::CommandBufferLevel level) {
    auto& bufferList = (level == core::CommandBufferLevel::Primary) ? pool.primaryCmds : pool.secondaryCmds;
    uint32_t& index = (level == core::CommandBufferLevel::Primary) ? pool.nextPrimary : pool.nextSecondary;

    // If we've already allocated enough buffers in previous frames, just return one (Hot path)
    if (index < bufferList.size()) {
        return commandBuffers_.ref(bufferList[index++]);
    }

    // Otherwise, we need to ask the driver for a new handle (Cold Path)
    auto newBuffer = commandBuffers_.create(pools_.get(pool.pool), level);
    bufferList.push_back(newBuffer);
    index++;
    return commandBuffers_.ref(newBuffer);
}
