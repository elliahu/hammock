#include <compare>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "gpu_task.hpp"

namespace hammock::renderer {

    PushConstantsBlock* BaseGpuTask::addPushConstantBlock(std::unique_ptr<PushConstantsBlock>&& block) {
        if (pushConstantsBlock_ != nullptr) {
            throw std::runtime_error("push constant block already set");
        }

        if (!block) return nullptr;

        pushConstantsBlock_ = std::move(block);
        return pushConstantsBlock_.get();
    }

    void BaseGpuTask::access(LogicalResourceAccess access) {
        logicalResourceAccesses_[access.handle].push_back(access);
    }
}  // namespace hammock::renderer
