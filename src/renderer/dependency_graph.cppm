module;
#include <cstdint>
#include <vector>
#include <memory>
#include <variant>

export module hammock_renderer:dependency_graph;

import hammock_core;
import :gpu_task;
import :socket;

namespace hammock::renderer {

    /// @struct LogicalImageResource
    /// Describes logical image resource
    export struct LogicalImageResource {
        core::ImageFormat format = core::ImageFormat::Undefined;
        core::ImageType type = core::ImageType::Type2D;
        core::ImageUsage usage; // TODO this can be inferred
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
    export struct LogicalBufferResource {
        core::BufferType type;
        core::BufferUsage usage;
        std::uint64_t instanceSize;
        std::uint32_t instanceCount;
        bool frameLocal = true;
        bool persistent = true;
    };

    /// @typedef LogicalResourceInterface
    /// Describes an option between LogicalImageResource and LogicalBufferResource
    export using LogicalResourceInterface = std::variant<
        std::monostate,
        LogicalImageResource,
        LogicalBufferResource
    >;

    /// @enum DependencyType
    /// @brief Describes a type of dependency between two tasks
    export enum class DependencyType {
        Execution, /// Task B must not begin until Task A has completed
        Debug, /// Validation only, hard error if inferred edge does not equal debug dge
    };

    /// @class DependencyGraph
    /// @brief Represents DAG of tasks and resources
    export class DependencyGraph final {
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

        std::vector<Dependency> dependencies_;

        struct LogicalResourceSlot {
            LogicalResourceInterface resource;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<LogicalResourceSlot> logicalResources_;
        std::int32_t firstFreeLogicalResourceSlot_ = -1;

    public:
        /// @brief Adds a resource to the graph
        LogicalResourceHandle addLogicalResource(LogicalResourceInterface iface = {});

        /// @brief Returns true if a handle is valid
        [[nodiscard]] bool isHandleValid(TaskHandle h) const;

        /// @brief Adds a gpu task to the graph
        TaskHandle addTask(std::unique_ptr<BaseGpuTask> &&task);

        /// @brief Removes a task from the graph
        void removeTask(TaskHandle h);

        /// @brief Declares an explicit execution dependency between two tasks.
        void connect(TaskHandle srcTaskHandle, TaskHandle dstTaskHandle, DependencyType dependencyType);

    };
}
