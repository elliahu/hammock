#include "gpu_task.hpp"

#include <compare>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>


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
        logicalResourceAccesses_.push_back(access);
    }
}  // namespace hammock::renderer
