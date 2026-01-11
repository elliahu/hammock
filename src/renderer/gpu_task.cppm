module;

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <compare>
#include <vulkan/vulkan.hpp>


export module hammock_renderer:gpu_task;

import :socket;
import :push_constants;
import hammock_core;


namespace hammock::renderer {

    // ********** Bse GPU task **********

    /// @struct TaskHandle
    /// @brief Returned when adding a task to TaskGraph.
    /// Represents a task inside a TaskGraph
    export struct TaskHandle {
        std::uint32_t index;
        std::uint32_t generation;
    };

    /// @interface BaseGpuTask
    /// Interface representing general GPU task that has input and outputs (sockets)
    class BaseGpuTask {
        friend class DependencyGraphCompiler;
    public:
        virtual ~BaseGpuTask() = default;

        /// @biref Add socket to the task
        /// @returns Sockets handle
        SocketHandle addSocket(std::unique_ptr<BaseSocket> &&socket);

        /// @brief Removes socket from the task
        void removeSocket(SocketHandle handle);

        /// @brief Add push constant block to the task
        /// @returns reference to the added push constant block
        PushConstantsBlock* addPushConstantBlock(std::unique_ptr<PushConstantsBlock> &&block);

        /// @brief Returns true if a handle is valid
        [[nodiscard]] bool isHandleValid(SocketHandle h) const;

    protected:
        void updateMetadata(BaseSocket& socket);

        struct Slot {
            std::unique_ptr<BaseSocket> socket;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<Slot> sockets_;
        std::int32_t firstFreeSlot_ = -1;
        std::unordered_map<std::string, SocketHandle> socketHandleResolutionMap_{};
        std::unique_ptr<PushConstantsBlock> pushConstantsBlock_{nullptr}; // Single push constant block allowed

        struct Metadata {
            // Inputs and outputs
            bool hasInputs = false;
            bool hasOutputs = false;
            std::vector<SocketHandle> inputs;
            std::vector<SocketHandle> outputs;

            // Depth stencil
            bool hasDepthStencil = false;
            SocketHandle depthStencilSocket;

            // Color targets
            bool hasColorTargets = false;
            std::vector<SocketHandle> colorTargetSockets{};

            // Sampled images
            bool hasSampledImages = false;
            std::vector<SocketHandle> sampledImageSockets{};

            // Storage images
            bool hasStorageImages = false;
            std::vector<SocketHandle> storageImageSockets{};

            // Uniform buffers
            bool hasUniformBuffers = false;
            std::vector<SocketHandle> uniformBuffers{};

            // Storage buffers
            bool hasStorageBuffers = false;
            std::vector<SocketHandle> storageBuffers{};
        };
        Metadata metadata_;
    };

    // ************ Graphics Task *************

    /// @class GraphicsTask
    /// @brief Specialized task that uses standard raster pipline.
    /// Vertex and fragment shaders required
    /// TODO use reflection to build the task from SPIR-V shader
    export class GraphicsTask final: public BaseGpuTask {
        friend class DependencyGraphCompiler;
    public:
        GraphicsTask(const std::string &vertexShaderFile, const std::string &fragmentShaderFile) : vertexShaderFile_(
                vertexShaderFile), fragmentShaderFile_(fragmentShaderFile) {
        }

    private:
        std::string vertexShaderFile_;
        std::string fragmentShaderFile_;
    };

    // *********** Compute Task ***********

    /// @class ComputeTask
    /// @brief Specialized task that uses GPU compute via compute shader
    /// /// TODO use reflection to build the task from SPIR-V shader
    export class ComputeTask final: public BaseGpuTask {
        friend class DependencyGraphCompiler;
    public:
        explicit ComputeTask(const std::string &computeShaderFile) : computeShaderFile_(computeShaderFile) {
        }

    private:
        std::string computeShaderFile_;
    };
}
