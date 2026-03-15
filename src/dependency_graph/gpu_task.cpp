#include "gpu_task.hpp"

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace hammock::graph {

    void GpuTask::access(LogicalResourceAccess access) { logicalResourceAccesses_.push_back(access); }

    void GpuTaskDeclBuilder::access(LogicalResourceAccess access) {
        logicalResourceAccesses_.push_back(access);
    }

    core::Handle<core::Image> GpuTaskExecContext::resolveImage(LogicalResourceHandle handle) {
        if (imageResolver_ == nullptr) {
            throw std::runtime_error("resolver is invalid. this should not happen.");
        }

        return imageResolver_(handle, frameIdx_);
    }

    core::Handle<core::Buffer> GpuTaskExecContext::resolveBuffer(LogicalResourceHandle handle) {
        if (bufferResolver_ == nullptr) {
            throw std::runtime_error("resolver is invalid. this should not happen.");
        }

        return bufferResolver_(handle, frameIdx_);
    }
}  // namespace hammock::graph
