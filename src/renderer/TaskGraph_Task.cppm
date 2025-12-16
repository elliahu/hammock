module;

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

export module hammock.renderer.task_graph:task;

import :socket;
import :push_constants;
import hammock.renderer.spirv_reflection;
import hammock.core.graphics_pipeline;
import hammock.core.compute_pipeline;

namespace hammock::renderer {

    /// @interface IGpuTask
    /// Interface representing general GPU task that has input and outputs (sockets)
    class IGpuTask {
    public:
        virtual ~IGpuTask() = default;

        /// @brief Execute the GPU task
        virtual void execute() = 0;

    protected:
        std::unordered_map<std::string, std::unique_ptr<ISocket>> sockets; // Direction is implied by the socket state
        std::unique_ptr<PushConstantsBlock> pushConstantsBlock{nullptr}; // Single push constant block allowed

    private:
        /// Add socket to the task
        std::unique_ptr<ISocket>& addSocket(std::unique_ptr<ISocket> socket) {
            if (sockets.contains(socket->getName())) {
                throw std::runtime_error("socket already exists");
            }

            sockets.emplace(socket->getName(), std::move(socket));
            return sockets[socket->getName()];
        }
    };

    /// @class GraphicsTask
    /// @brief Specialized task that uses standard raster pipline.
    /// Vertex and fragment shaders required
    class GraphicsTask : public IGpuTask {
    public:
        GraphicsTask(const std::string &vertexShaderFile, const std::string &fragmentShaderFile) {
            /// TODO use reflection to create the task
        }

    private:
        std::unique_ptr<core::GraphicsPipeline> pipeline{nullptr};
    };

    /// @class ComputeTask
    /// @brief Specialized task that uses GPU compute via compute shader
    class ComputeTask : public IGpuTask {
    public:
        explicit ComputeTask(const std::string &computeShaderFile) {
            /// TODO use reflection to create the task
        }
    private:
        std::unique_ptr<core::ComputePipeline> pipeline{nullptr};
    };
}
