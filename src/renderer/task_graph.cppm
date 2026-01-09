module;
#include <cstdint>
#include <vector>
#include <memory>

export module hammock_renderer:task_graph;

import :gpu_task;

namespace hammock::renderer {
    /// @class TaskGraph
    /// @brief Represents DAG of tasks and resources
    export class TaskGraph final {
        struct Slot {
            std::unique_ptr<BaseGpuTask> task;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<Slot> tasks_;
        std::int32_t firstFreeSlot_ = -1;

        struct SocketConnection {
            TaskHandle srcTaskHandle;
            SocketHandle srcSocketHandle;
            TaskHandle dstTaskHandle;
            SocketHandle dstSocketHandle;
        };

        std::vector<SocketConnection> connections_;

    public:
        /// @brief Returns true if a handle is valid
        [[nodiscard]] bool isHandleValid(TaskHandle h) const;

        /// @brief Adds a gpu task to the graph
        TaskHandle add(std::unique_ptr<BaseGpuTask> &&task);

        /// @brief Removes a task from the graph
        void remove(TaskHandle h);

        /// @brief Connect sockets of two tasks - thus marking them as using the same resource (producer and consumer)
        void connect(TaskHandle srcTaskHandle, SocketHandle srcSocketHandle, TaskHandle dstTaskHandle,
                     SocketHandle dstSocketHandle);
    };


    /// @class GraphCompiler
    /// @brief Outputs compiled graph
    export class GraphCompiler final {
    };

    /// @struct CompiledGraph
    /// @brief Represents output of graph compiler
    export struct CompiledGraph final {
    };


    /// @class GraphExecutor
    /// @brief Executes compiled graph, orchestrates the recording and submission of command buffers
    export class GraphExecutor final {
    };
}
