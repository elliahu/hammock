module;
#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>

module hammock_renderer;

namespace hammock::renderer {
    LogicalResourceHandle DependencyGraph::addLogicalResource(LogicalResourceInterface iface) {
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
        slot.resource = iface;
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        return { idx, slot.generation };
    }

    bool DependencyGraph::isHandleValid(TaskHandle h) const {
        return h.index < tasks_.size() &&
               tasks_[h.index].generation == h.generation &&
               tasks_[h.index].active;
    }

    TaskHandle DependencyGraph::addTask(std::unique_ptr<BaseGpuTask> &&task) {
        std::uint32_t idx;

        if (firstFreeTaskSlot_ != -1) {
            // Reuse an empty slot
            idx = firstFreeTaskSlot_;
            firstFreeTaskSlot_ = static_cast<std::int32_t>(tasks_[idx].nextFreeSlot);
        } else {
            // No empty slots, grow the vector
            idx = static_cast<uint32_t>(tasks_.size());
            tasks_.emplace_back();
        }

        TaskSlot& slot = tasks_[idx];
        slot.task = std::move(task);
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        return { idx, slot.generation };
    }

    void DependencyGraph::removeTask(TaskHandle h) {
        if (isHandleValid(h)) {
            TaskSlot& slot = tasks_[h.index];
            slot.active = false;
            slot.generation++; // This invalidates all existing handles to this slot

            // Push this slot onto the front of the free list
            slot.nextFreeSlot = firstFreeTaskSlot_;
            firstFreeTaskSlot_ = static_cast<std::int32_t>(h.index);
        }
    }

    void DependencyGraph::connect(TaskHandle srcTaskHandle, TaskHandle dstTaskHandle, DependencyType dependencyType) {
        dependencies_.push_back({srcTaskHandle, dstTaskHandle, dependencyType});
    }
}
