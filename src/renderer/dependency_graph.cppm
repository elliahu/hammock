module;
#include <cstdint>
#include <vector>
#include <memory>

export module hammock_renderer:dependency_graph;

import :gpu_task;
import :socket;

namespace hammock::renderer {

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
        LogicalResourceHandle addResource(LogicalResourceInterface iface = {});

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
