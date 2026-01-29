#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

#include "base_resource.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "gpu_task.hpp"
#include "hammock_core.hpp"
#include "math.hpp"

namespace hammock::renderer {

    /// @struct LogicalImageResource
    /// Describes logical image resource
    struct LogicalImageResource {
        core::ImageFormat format = core::ImageFormat::Undefined;
        core::ImageType type = core::ImageType::Type2D;
        core::ImageUsage usage;  // TODO this can be inferred
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t channels = 4;
        std::uint32_t depth = 1;
        std::uint32_t levels = 1;
        std::uint32_t layers = 1;
        bool frameLocal = true;
        bool persistent = true;
    };

    /// @struct LogicalBufferResource
    /// Describes logical buffer resource
    struct LogicalBufferResource {
        core::BufferType type;
        core::BufferUsage usage;
        std::uint64_t instanceSize;
        std::uint32_t instanceCount;
        bool frameLocal = true;
        bool persistent = true;
    };

    /// @typedef LogicalResourceInterface
    /// Describes an option between LogicalImageResource and LogicalBufferResource
    using LogicalResourceInterface =
        std::variant<LogicalImageResource, LogicalBufferResource>;

    /// @enum DependencyType
    /// @brief Describes a type of dependency between two tasks
    enum class DependencyType {
        Execution,  /// Task B must not begin until Task A has completed
        Debug,      /// Asserts during compilation that A -> B exists, if not, throws
    };

    /// @class DependencyGraph
    /// @brief Represents DAG of tasks and resources
    class DependencyGraph final {
        friend class DependencyGraphCompiler;
        struct TaskSlot {
            std::unique_ptr<BaseGpuTask> task;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<TaskSlot> tasks_;
        std::int32_t firstFreeTaskSlot_ = -1;

        struct Dependency {
            TaskHandle srcTaskHandle;
            TaskHandle dstTaskHandle;
            DependencyType dependencyType;
        };

        std::vector<Dependency> explicitDependencies_;

        struct LogicalResourceSlot {
            LogicalResourceInterface resource;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<LogicalResourceSlot> logicalResources_;
        std::int32_t firstFreeLogicalResourceSlot_ = -1;

        bool isPresentTaskSet_ = false;
        TaskHandle presentTaskHandle_;

        struct InitCopyBuffer {
            std::reference_wrapper<core::Buffer> buffer;
        };

        struct InitCopyImage {
            std::reference_wrapper<core::Image> image;
        };

        struct InitClearImage {
            std::array<float, 4> clearColor;
            std::array<float, 2> clearDepthStencil;
        };

        using InitResourceInterface = std::variant<InitCopyBuffer, InitCopyImage, InitClearImage>;

        std::vector<InitResourceInterface> resourceInits_{};

       public:
        /// @brief Adds a resource to the graph
        LogicalResourceHandle addLogicalResource(LogicalResourceInterface iface = {});

        /// @brief Copy buffer contents into target resource
        void initCopyBuffer(LogicalResourceHandle target, core::Buffer& src);

        /// @brief Copy image contents into target resource
        void initCopyImage(LogicalResourceHandle target, core::Image& src);

        /// @brief Clear target image
        /// @pre target must be image, if not, throws
        void initClearImage(LogicalResourceHandle target, std::array<float, 4> clearColor, std::array<float, 2> clearDepthStencil = {1.f, 1.f});

        /// @brief Returns true if a handle is valid
        [[nodiscard]] bool isHandleValid(TaskHandle h) const;

        /// @brief Adds a gpu task to the graph
        TaskHandle addTask(std::unique_ptr<BaseGpuTask>&& task);

        /// @brief Removes a task from the graph
        void removeTask(TaskHandle h);

        /// @brief Declares an explicit execution dependency between two tasks.
        void dependency(TaskHandle srcTaskHandle, TaskHandle dstTaskHandle, DependencyType dependencyType);

        /// @brief Marks this task as present.
        /// This will end the execution of the render graph when the task marked present is executed
        void present(TaskHandle presentTaskHandle);
    };
}  // namespace hammock::renderer
