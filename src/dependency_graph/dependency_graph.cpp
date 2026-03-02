#include "dependency_graph.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "gpu_task.hpp"


namespace hammock::graph {
    LogicalResourceHandle DependencyGraph::resource(LogicalResourceInterface iface) {
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
        return {idx, slot.generation};
    }

    bool DependencyGraph::isHandleValid(TaskHandle h) const {
        return h.index < tasks_.size() && tasks_[h.index].generation == h.generation &&
               tasks_[h.index].active;
    }

    TaskHandle DependencyGraph::task(std::unique_ptr<BaseGpuTask>&& task) {
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

    void DependencyGraph::present(TaskHandle presentTaskHandle, LogicalResourceHandle presentResourceHandle) {
        presentTaskHandle_ = std::move(presentTaskHandle);
        presentResourceHandle_ = std::move(presentResourceHandle);
        isPresentSet_ = true;
    }

    void DependencyGraph::initCopyBuffer(LogicalResourceHandle target, core::Buffer& src) {
        resourceInits_[target] = InitCopyBuffer{.buffer = src};
    }

    void DependencyGraph::initCopyImage(LogicalResourceHandle target, core::Image& src) {
        resourceInits_[target] = InitCopyImage{.image = src};
    }

    void DependencyGraph::initClearImage(LogicalResourceHandle target, std::array<float, 4> clearColor,
        std::array<float, 2> clearDepthStencil) {
        if (auto image = std::get_if<LogicalImageResource>(&logicalResources_[target.index].resource)) {
            resourceInits_[target] = InitClearImage{
                .clearColor = clearColor,
                .clearDepthStencil = clearDepthStencil,
            };
        } else {
            throw std::runtime_error("cannot clear image, target is not image");
        }
    }

    LogicalResourceHandle DependencyGraph::image(LogicalImageResource imageResource) {
        return resource(imageResource);
    }

    LogicalResourceHandle DependencyGraph::buffer(LogicalBufferResource bufferResource) {
        return resource(bufferResource);
    }
    
}  // namespace hammock::renderer
