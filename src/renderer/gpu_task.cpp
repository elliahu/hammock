module;

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <compare>
#include <vulkan/vulkan.hpp>


module hammock_renderer;

namespace hammock::renderer {
    SocketHandle BaseGpuTask::addSocket(std::unique_ptr<BaseSocket> &&socket) {
        std::uint32_t idx;

        if (firstFreeSlot_ != -1) {
            // Reuse an empty slot
            idx = firstFreeSlot_;
            firstFreeSlot_ = static_cast<std::int32_t>(sockets_[idx].nextFreeSlot);
        } else {
            // No empty slots, grow the vector
            idx = static_cast<uint32_t>(sockets_.size());
            sockets_.emplace_back();
        }

        Slot& slot = sockets_[idx];
        slot.task = std::move(socket);
        slot.active = true;
        // Note: We don't increment generation here; we do it on removal
        return { idx, slot.generation };
    }

    void BaseGpuTask::removeSocket(SocketHandle handle) {
        if (isHandleValid(handle)) {
            Slot& slot = sockets_[handle.index];
            slot.active = false;
            slot.generation++; // This invalidates all existing handles to this slot

            // Push this slot onto the front of the free list
            slot.nextFreeSlot = firstFreeSlot_;
            firstFreeSlot_ = static_cast<std::int32_t>(handle.index);
        }
    }

    PushConstantsBlock* BaseGpuTask::
    addPushConstantBlock(std::unique_ptr<PushConstantsBlock> &&block) {
        if (pushConstantsBlock_ != nullptr) {
            throw std::runtime_error("push constant block already set");
        }

        if (!block) return nullptr;

        pushConstantsBlock_ = std::move(block);
        return pushConstantsBlock_.get();
    }

    bool BaseGpuTask::isHandleValid(SocketHandle h) const {
        return h.index < sockets_.size() &&
               sockets_[h.index].generation == h.generation &&
               sockets_[h.index].active;
    }
}
