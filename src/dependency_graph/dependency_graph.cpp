#include "dependency_graph.hpp"

#include <cstdint>
#include <vector>

#include "gpu_task.hpp"

namespace hammock::graph {
    LogicalResourceHandle DependencyGraph::resource(LogicalResourceInterface riface) {
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

    bool DependencyGraph::isHandleValid(TaskHandle h) const {
        return h.index < tasks_.size() && tasks_[h.index].generation == h.generation &&
               tasks_[h.index].active;
    }

    TaskHandle DependencyGraph::task(GpuTask&& task) {

        std::uint32_t idx;

        if (firstFreeTaskSlot_ != -1) {
            // Reuse an empty slot
            idx = firstFreeTaskSlot_;
            firstFreeTaskSlot_ = static_cast<std::int32_t>(tasks_[idx].nextFreeSlot);
        } else {
            // No empty slots, grow the vector
            idx = static_cast<uint32_t>(tasks_.size());
            tasks_.resize(tasks_.size() + 1);
        }

        TaskSlot& slot = tasks_[idx];
        slot.task = std::move(task);
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        return {idx, slot.generation};
    }

    void DependencyGraph::remove(TaskHandle h) {
        if (isHandleValid(h)) {
            TaskSlot& slot = tasks_[h.index];
            slot.active = false;
            slot.generation++;  // This invalidates all existing handles to this slot

            // Push this slot onto the front of the free list
            slot.nextFreeSlot = firstFreeTaskSlot_;
            firstFreeTaskSlot_ = static_cast<std::int32_t>(h.index);
        }
    }

    void DependencyGraph::remove(LogicalResourceHandle h) {
        // TODO Implement removing logical resources
    }

    void DependencyGraph::dependency(
        TaskHandle srcTaskHandle, TaskHandle dstTaskHandle, DependencyType dependencyType) {
        explicitDependencies_.push_back({srcTaskHandle, dstTaskHandle, dependencyType});
    }

    LogicalResourceHandle DependencyGraph::image(LogicalImageResource imageResource) {
        return resource(imageResource);
    }

    LogicalResourceHandle DependencyGraph::buffer(LogicalBufferResource bufferResource) {
        return resource(bufferResource);
    }

    void DependencyGraph::root(TaskHandle task) {
        root_ = task;
        hasRoot_ = true;
    }
}  // namespace hammock::graph
