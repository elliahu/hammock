#include "render_graph.hpp"

#include <cstdint>
#include <vector>

#include "render_pass.hpp"

namespace hammock::graph {
    LogicalResourceHandle RenderGraph::resource(LogicalResourceInterface riface) {
        std::uint32_t idx;

        if (firstFreeLogicalResourceSlot_ != -1) {
            // Reuse an empty slot
            idx = firstFreeLogicalResourceSlot_;
            firstFreeLogicalResourceSlot_ = static_cast<std::int32_t>(logicalResources_[idx].nextFreeSlot);
        } else {
            // No empty slots, grow the vector
            idx = static_cast<uint32_t>(logicalResources_.size());
            logicalResources_.emplace_back();
        }

        LogicalResourceSlot& slot = logicalResources_[idx];
        slot.resource = riface;
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        LogicalResourceHandle handle {idx, slot.generation};
        return handle;
    }

    bool RenderGraph::isHandleValid(RenderPassHandle h) const {
        return h.index < passes_.size() && passes_[h.index].generation == h.generation &&
               passes_[h.index].active;
    }

    RenderPassHandle RenderGraph::pass(RenderPass&& pass) {

        std::uint32_t idx;

        if (firstFreePassSlot_ != -1) {
            // Reuse an empty slot
            idx = firstFreePassSlot_;
            firstFreePassSlot_ = static_cast<std::int32_t>(passes_[idx].nextFreeSlot);
        } else {
            // No empty slots, grow the vector
            idx = static_cast<uint32_t>(passes_.size());
            passes_.resize(passes_.size() + 1);
        }

        RenderPassSlot& slot = passes_[idx];
        slot.pass = std::move(pass);
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        return {idx, slot.generation};
    }

    void RenderGraph::remove(RenderPassHandle h) {
        if (isHandleValid(h)) {
            RenderPassSlot& slot = passes_[h.index];
            slot.active = false;
            slot.generation++;  // This invalidates all existing handles to this slot

            // Push this slot onto the front of the free list
            slot.nextFreeSlot = firstFreePassSlot_;
            firstFreePassSlot_ = static_cast<std::int32_t>(h.index);
        }
    }

    void RenderGraph::remove(LogicalResourceHandle h) {
        // TODO Implement removing logical resources
    }

    void RenderGraph::dependency(
        RenderPassHandle srcPassHandle, RenderPassHandle dstPassHandle, DependencyType dependencyType) {
        explicitDependencies_.push_back({srcPassHandle, dstPassHandle, dependencyType});
    }

    LogicalResourceHandle RenderGraph::image(LogicalImageResource imageResource) {
        return resource(imageResource);
    }

    LogicalResourceHandle RenderGraph::buffer(LogicalBufferResource bufferResource) {
        return resource(bufferResource);
    }

    void RenderGraph::root(RenderPassHandle pass) {
        root_ = pass;
        hasRoot_ = true;
    }
}  // namespace hammock::graph
