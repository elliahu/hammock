#include "gpu_task.hpp"

#include <compare>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>


namespace hammock::graph {

    void GpuTask::access(LogicalResourceAccess access) {
        logicalResourceAccesses_.push_back(access);
    }


    core::ResourceHandle GpuTaskExecContext::resolveResource(LogicalResourceHandle handle) {
        if (resolver_ == nullptr) {
            throw std::runtime_error("resolver is invalid. this should not happen.");
        }

        return resolver_(handle, frameIdx_);
    }
    
    void GpuTaskDeclBuilder::access(LogicalResourceAccess access) {
        logicalResourceAccesses_.push_back(access);
    }
}  // namespace hammock::graph
