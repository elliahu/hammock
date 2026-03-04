#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

#include "core/base_resource.hpp"
#include "core/buffer.hpp"
#include "core/command_buffer.hpp"
#include "core/image.hpp"
#include "gpu_task.hpp"
#include "utils/math.hpp"


namespace hammock::graph {

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
    using LogicalResourceInterface = std::variant<LogicalImageResource, LogicalBufferResource>;

    /// @enum DependencyType
    /// @brief Describes a type of dependency between two tasks
    enum class DependencyType {
        Execution,  /// Task B must not begin until Task A has completed
        Debug,      /// Asserts during compilation that A -> B exists, if not, throws
    };

    /// @class DependencyGraph
    /// @brief Represents DAG of tasks and resources
    /// TODO alow import of externally owned resources
    class DependencyGraph final {
        friend class DependencyGraphCompiler;
        friend struct CompiledLogicalResource;

        struct TaskSlot {
            GpuTask task;
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

        TaskHandle root_;
        bool hasRoot_{false};

       public:
        /// @brief Adds a resource to the graph
        [[nodiscard]] LogicalResourceHandle resource(LogicalResourceInterface riface);

        /// @brief Adds an image to the graph
        /// @note Wrapper around addResource
        [[nodiscard]] LogicalResourceHandle image(LogicalImageResource imageResource);

        /// @brief Adds a buffer to the graph
        /// @note Wrapper around addResource
        [[nodiscard]] LogicalResourceHandle buffer(LogicalBufferResource bufferResource);

        /// @brief Copy buffer contents into target resource
        void initCopyBuffer(LogicalResourceHandle target, core::Buffer& src);

        /// @brief Copy image contents into target resource
        void initCopyImage(LogicalResourceHandle target, core::Image& src);

        /// @brief Clear target image
        /// @pre target must be image, if not, throws
        void initClearImage(LogicalResourceHandle target, std::array<float, 4> clearColor,
            std::array<float, 2> clearDepthStencil = {1.f, 1.f});

        /// @brief Returns true if a handle is valid
        [[nodiscard]] bool isHandleValid(TaskHandle h) const;

        /// @brief Adds a gpu task to the graph
        [[nodiscard]] TaskHandle task(GpuTask&& task);

        /// @brief Removes a task from the graph
        void remove(TaskHandle h);

        /// @brief Removes a resource from the graph
        void remove(LogicalResourceHandle h);

        /// @brief Declares an explicit execution dependency between two tasks.
        void dependency(TaskHandle srcTaskHandle, TaskHandle dstTaskHandle, DependencyType dependencyType);

        /// @brief Marks this task as root (last task)
        /// Tasks that do not contribute to the root task might get culled out during compilation.
        void root(TaskHandle task);
    };
}  // namespace hammock::graph
