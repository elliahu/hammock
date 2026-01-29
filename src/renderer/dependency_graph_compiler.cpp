#include "dependency_graph_compiler.hpp"

#include <cstdint>
#include <memory>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "base_resource.hpp"
#include "buffer.hpp"
#include "dependency_graph.hpp"
#include "gpu_task.hpp"
#include "hammock_core.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "swapchain.hpp"

namespace hammock::renderer {

    ReadWriteIntent DependencyGraphCompiler::determineReadWriteIntent(AccessInterface accessIface) {
        if (auto imageAccess = std::get_if<ImageAccess>(&accessIface)) {
            switch (*imageAccess) {
                case ImageAccess::ColorAttachmentWrite:
                case ImageAccess::DepthAttachmentWrite:
                    return ReadWriteIntent::Write;
                case ImageAccess::StorageReadWrite:
                    return ReadWriteIntent::ReadWrite;
                case ImageAccess::SampledRead:
                    return ReadWriteIntent::Read;
                default:
                    throw std::runtime_error("could not determine read write intent for image");
            }
        }

        else if (auto bufferAccess = std::get_if<BufferAccess>(&accessIface)) {
            switch (*bufferAccess) {
                case BufferAccess::UniformBufferRead:
                    return ReadWriteIntent::Read;
                case BufferAccess::StorageBufferReadWrite:
                    return ReadWriteIntent::ReadWrite;
                default:
                    throw std::runtime_error("could not determine read write intent for buffer");
            }
        }

        throw std::runtime_error("could not determine read write intent");
    }

    void DependencyGraphCompiler::buildGraphNodes(DependencyGraph& dependencyGraph) {
        nodes_.resize(dependencyGraph.tasks_.size());
        for (std::uint32_t idx = 0; idx < dependencyGraph.tasks_.size(); idx++) {
            auto task = dependencyGraph.tasks_[idx].task.get();
            nodes_[idx].task = task;

            // This will fill out the logicalResourceUses_
            for (auto access : task->logicalResourceAccesses_) {
                logicalResourceUses_[access.handle].push_back(TaskHandleLogicalResourceAccessPair{
                    .task = TaskHandle{.index = idx, .generation = dependencyGraph.tasks_[idx].generation},
                    .access = access.access});
            }
        }

        handleExplicitDependencies(dependencyGraph);

        // Detect hazards
        for (auto accessPair : logicalResourceUses_) {
            auto resourceHandle = accessPair.first;
            auto accesses = accessPair.second;

            for (auto&& [a, b] : std::views::zip(accesses, std::views::drop(accesses, 1))) {
                auto intentA = determineReadWriteIntent(a.access);
                auto intentB = determineReadWriteIntent(b.access);
                if (isHazard(intentA, intentB)) {
                    addEdge(a.task.index, b.task.index);
                }
            }
        }

        assertDebugEdges(dependencyGraph);
    }

    void DependencyGraphCompiler::addEdge(std::uint32_t aidx, std::uint32_t bidx) {
        if (!std::ranges::contains(nodes_[aidx].outgoing, bidx)) {
            nodes_[aidx].outgoing.push_back(bidx);
            nodes_[bidx].indegree++;
        }
    }

    void DependencyGraphCompiler::handleExplicitDependencies(DependencyGraph& dependencyGraph) {
        for (auto& dependency : dependencyGraph.explicitDependencies_) {
            // We are using the handle index directly later so we need to check if the handle is valid
            // (generations match)
            if (!dependencyGraph.isHandleValid(dependency.srcTaskHandle) ||
                !dependencyGraph.isHandleValid(dependency.dstTaskHandle)) {
                throw std::runtime_error("invalid task handle");
            }

            if (dependency.dependencyType == DependencyType::Execution) {
                addEdge(dependency.srcTaskHandle.index, dependency.dstTaskHandle.index);
            }
        }
    }

    void DependencyGraphCompiler::compileLogicalResources(DependencyGraph& dependencyGraph) {
        for (std::uint32_t idx = 0; idx < dependencyGraph.logicalResources_.size(); idx++) {
            auto& logicalResourceIface = dependencyGraph.logicalResources_[idx].resource;

            CompiledLogicalResource compiledLogicalResource{};
            compiledLogicalResource.origin = LogicalResourceHandle{
                .index = idx, .generation = dependencyGraph.logicalResources_[idx].generation};

            // Determine the type of the resource
            // Image resource
            if (auto logicalImageResource = std::get_if<LogicalImageResource>(&logicalResourceIface)) {
                std::uint32_t numOfCopies = 1;
                if (logicalImageResource->frameLocal) {
                    numOfCopies = core::SwapChain::MAX_FRAMES_IN_FLIGHT;
                }

                for (int i = 0; i < numOfCopies; i++) {
                    core::ResourceHandle handle = createPhysicalImageResource(logicalImageResource);
                    compiledLogicalResource.handles.push_back(handle);
                }
            }

            // Buffer resource
            if (auto logicalBufferResource = std::get_if<LogicalBufferResource>(&logicalResourceIface)) {
                std::uint32_t numOfCopies = 1;
                if (logicalBufferResource->frameLocal) {
                    numOfCopies = core::SwapChain::MAX_FRAMES_IN_FLIGHT;
                }

                for (int i = 0; i < numOfCopies; i++) {
                    core::ResourceHandle handle = createPhysicalBufferResource(logicalBufferResource);
                    compiledLogicalResource.handles.push_back(handle);
                }
            }

            compiledDependencyGraph_.compiledLogicalResources.push_back(compiledLogicalResource);
        }
    }

    core::ResourceHandle DependencyGraphCompiler::createPhysicalImageResource(
        LogicalImageResource* logicalImageResource) {
        return core::ResourceManager::getInstance().createResource<core::Image>(core::ImageDesc{
            .width = logicalImageResource->width,
            .height = logicalImageResource->height,
            .channels = logicalImageResource->channels,
            .depth = logicalImageResource->depth,
            .layers = logicalImageResource->layers,
            .format = logicalImageResource->format,
            .usage = logicalImageResource->usage,
            .type = logicalImageResource->type,
        });
    }
    core::ResourceHandle DependencyGraphCompiler::createPhysicalBufferResource(
        LogicalBufferResource* logicalBufferResource) {
        return core::ResourceManager::getInstance().createResource<core::Buffer>(
            core::BufferDesc{.type = logicalBufferResource->type,
                .usage = logicalBufferResource->usage,
                .instanceSize = logicalBufferResource->instanceSize,
                .instanceCount = logicalBufferResource->instanceCount});
    }

    bool DependencyGraphCompiler::isHazard(ReadWriteIntent a, ReadWriteIntent b) {
        if (a == ReadWriteIntent::Read && b == ReadWriteIntent::Read) {
            return false;
        }
        return true;  // Everything else is hazard
    }

    void DependencyGraphCompiler::assertDebugEdges(DependencyGraph& dependencyGraph) {
        for (auto edge : dependencyGraph.explicitDependencies_) {
            // Make sure there is an edge from a to b (A -> B)
            if (edge.dependencyType == DependencyType::Debug &&
                !std::ranges::contains(nodes_[edge.srcTaskHandle.index].outgoing, edge.dstTaskHandle.index)) {
                throw std::runtime_error("expected edge not found during assertion");
            }
        }
    }
    void DependencyGraphCompiler::determineExecutionLevels() {
        std::queue<std::uint32_t> ready{};

        // Init ready set
        for (std::uint32_t i = 0; i < nodes_.size(); i++) {
            auto& n = nodes_[i];
            if (n.indegree == 0) {
                ready.push(i);
            }
        }

        while (!ready.empty()) {
            std::vector<std::uint32_t> level{};
            std::size_t count = ready.size();

            // All tasks currently ready can run in parallel
            for (std::size_t i = 0; i < count; ++i) {
                auto t = ready.front();
                ready.pop();
                level.push_back(t);

                // Remove this task from graph
                for (auto next : nodes_[t].outgoing) {
                    if (--nodes_[next].indegree == 0) {
                        ready.push(next);
                    }
                }
            }
            compiledDependencyGraph_.executionLevels.push_back(level);
        }
    }
}  // namespace hammock::renderer
