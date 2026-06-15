#pragma once

#include "core/swapchain.hpp"
#include "core/utilities.hpp"
#include "renderer/render_snapshot.hpp"

namespace hammock::app {
    /// @struct FrameArena
    /// @brief Memory arena for a frame data
    struct FrameArena {
        std::vector<std::uint8_t> buffer;
        size_t offset = 0;

        void reset() {
#ifndef NDEBUG
            // Fill the buffer with garbage so that the thread crashes instantly on accessing it
            // This is here as a bug-catching mechanism
            std::memset(buffer.data(), 0xFD, buffer.size());
#endif

            // Reset offset to right after the header
            offset = sizeof(renderer::RenderSnapshotHeader);
        }

        // Linear allocation inside this frame's block
        std::uint8_t* allocate(size_t size, size_t alignment) {
            size_t alignedOffset = (offset + alignment - 1) & ~(alignment - 1);
            if (alignedOffset + size > buffer.size()) {
                // Panic or dynamically grow. In ideal case, preallocation is large enought si this never
                // happens.
                core::Logger::warn("%s", "Allocated arena size exceeded, growing ...");
                // Grow
                buffer.resize(buffer.size() * 2);
            }
            offset = alignedOffset + size;
            return buffer.data() + alignedOffset;
        }

        void logDebugMetrics() {
            float usagePercent = (static_cast<float>(offset) / buffer.size()) * 100.0f;
            core::Logger::debug("Peak frame size: %zu bytes (%.2f%% used)", offset, usagePercent);
        }
    };

    /// @class RenderRingBuffer
    /// @brief Ring buffer of frame arenas. 16MB default size
    class RenderRingBuffer {
       public:
        RenderRingBuffer(size_t arenaSizePerFrame = 16 * 1024 * 1024) {  // 16MB default
            for (uint32_t i = 0; i < core::SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
                arenas[i].buffer.resize(arenaSizePerFrame);
                arenas[i].reset();
            }
        }

        FrameArena& getFrameArena(uint64_t frameNumber) {
            return arenas[frameNumber % core::SwapChain::MAX_FRAMES_IN_FLIGHT];
        }

       private:
        FrameArena arenas[core::SwapChain::MAX_FRAMES_IN_FLIGHT];
    };
}  // namespace hammock::app
