module;

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <compare>
#include <vulkan/vulkan.hpp>


export module hammock.renderer.task_graph:task;

import :socket;
import :push_constants;
import hammock.renderer.spirv_reflection;
import hammock.core.graphics_pipeline;
import hammock.core.compute_pipeline;
import hammock.core.command_buffer;


namespace hammock::renderer {
    /// @interface IGpuTask
    /// Interface representing general GPU task that has input and outputs (sockets)
    class IGpuTask {
    public:
        virtual ~IGpuTask() = default;

        /// @brief Execute the GPU task
        //virtual void execute(std::unique_ptr<core::CommandBuffer> &commandBuffer) = 0;

        /// @biref Add socket to the task
        /// @returns reference to the added socket
        std::unique_ptr<ISocket> &addSocket(std::unique_ptr<ISocket> &&socket) {
            if (sockets.contains(socket->getName())) {
                throw std::runtime_error("socket already exists");
            }
            std::string name = socket->getName();
            sockets.emplace(socket->getName(), std::move(socket));
            return sockets[name];
        }

        /// @brief Add push constant block to the task
        /// @returns reference to the added push constant block
        std::unique_ptr<PushConstantsBlock> &addPushConstantBlock(std::unique_ptr<PushConstantsBlock> &&block) {
            if (pushConstantsBlock != nullptr) {
                throw std::runtime_error("push constant block already set");
            }
            pushConstantsBlock = std::move(block);
            return pushConstantsBlock;
        }

    protected:
        std::unordered_map<std::string, std::unique_ptr<ISocket> > sockets; // Direction is implied by the socket state
        std::unique_ptr<PushConstantsBlock> pushConstantsBlock{nullptr}; // Single push constant block allowed
    };

    /// @class GraphicsTask
    /// @brief Specialized task that uses standard raster pipline.
    /// Vertex and fragment shaders required
    export class GraphicsTask : public IGpuTask {
    public:
        GraphicsTask(const std::string &vertexShaderFile, const std::string &fragmentShaderFile) : vertexShaderFile(
                vertexShaderFile), fragmentShaderFile(fragmentShaderFile) {
        }

    private:
        std::unique_ptr<core::GraphicsPipeline> pipeline{nullptr};
        std::string vertexShaderFile;
        std::string fragmentShaderFile;
    };

    /// @class ComputeTask
    /// @brief Specialized task that uses GPU compute via compute shader
    export class ComputeTask : public IGpuTask {
    public:
        explicit ComputeTask(const std::string &computeShaderFile) : computeShaderFile(computeShaderFile) {
        }

    private:
        std::unique_ptr<core::ComputePipeline> pipeline{nullptr};
        std::string computeShaderFile;
    };
}
