module;
#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>

module hammock_renderer;

namespace hammock::renderer {
    bool TaskGraph::isHandleValid(TaskHandle h) const {
        return h.index < tasks_.size() &&
               tasks_[h.index].generation == h.generation &&
               tasks_[h.index].active;
    }

    TaskHandle TaskGraph::add(std::unique_ptr<BaseGpuTask> &&task) {
        std::uint32_t idx;

        if (firstFreeSlot_ != -1) {
            // Reuse an empty slot
            idx = firstFreeSlot_;
            firstFreeSlot_ = static_cast<std::int32_t>(tasks_[idx].nextFreeSlot);
        } else {
            // No empty slots, grow the vector
            idx = static_cast<uint32_t>(tasks_.size());
            tasks_.emplace_back();
        }

        Slot& slot = tasks_[idx];
        slot.task = std::move(task);
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        return { idx, slot.generation };
    }

    void TaskGraph::remove(TaskHandle h) {
        if (isHandleValid(h)) {
            Slot& slot = tasks_[h.index];
            slot.active = false;
            slot.generation++; // This invalidates all existing handles to this slot

            // Push this slot onto the front of the free list
            slot.nextFreeSlot = firstFreeSlot_;
            firstFreeSlot_ = static_cast<std::int32_t>(h.index);
        }
    }

    void TaskGraph::connect(TaskHandle srcTaskHandle, SocketHandle srcSocketHandle, TaskHandle dstTaskHandle,
        SocketHandle dstSocketHandle) {
        if (!isHandleValid(srcTaskHandle) || !isHandleValid(dstTaskHandle)) {
            throw std::runtime_error("invalid task handle");
        }

        auto srcTask = tasks_[srcTaskHandle.index].task.get();
        auto dstTask = tasks_[dstTaskHandle.index].task.get();

        if (!srcTask->isHandleValid(srcSocketHandle) || !dstTask->isHandleValid(dstSocketHandle)) {
            throw std::runtime_error("invalid socket handle");
        }

        connections_.push_back({
            .srcTaskHandle = srcTaskHandle,
            .srcSocketHandle = srcSocketHandle,
            .dstTaskHandle = dstTaskHandle,
            .dstSocketHandle = dstSocketHandle
        });
    }
}
