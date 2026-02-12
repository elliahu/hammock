#include "dependency_graph_compiler.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "core/base_resource.hpp"
#include "core/buffer.hpp"
#include "core/compute_pipeline.hpp"
#include "dependency_graph.hpp"
#include "gpu_task.hpp"
#include "core/image.hpp"
#include "core/resource_manager.hpp"
#include "core/swapchain.hpp"

namespace hammock::renderer {

    DependencyGraphCompiler::DependencyGraphCompiler(core::VulkanContext& ctx) : ctx_(ctx) {}

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
                case BufferAccess::UniformRead:
                    return ReadWriteIntent::Read;
                case BufferAccess::StorageReadWrite:
                    return ReadWriteIntent::ReadWrite;
                default:
                    throw std::runtime_error("could not determine read write intent for buffer");
            }
        }

        throw std::runtime_error("could not determine read write intent");
    }

    void DependencyGraphCompiler::buildGraphNodes(DependencyGraph& dependencyGraph) {
        nodes_.resize(dependencyGraph.tasks_.size());
        compiledDependencyGraph_.compiledTasks.resize(dependencyGraph.tasks_.size());
        for (std::uint32_t idx = 0; idx < dependencyGraph.tasks_.size(); idx++) {
            auto task = dependencyGraph.tasks_[idx].task.get();
            nodes_[idx].task = task;

            // If the task is present task, mark it
            TaskHandle taskHandle{
                .index = idx,
                .generation = dependencyGraph.tasks_[idx].generation,
            };

            if (taskHandle == dependencyGraph.presentTaskHandle_) {
                compiledDependencyGraph_.presentTaskIdx = static_cast<std::int32_t>(idx);
            }

            // This will fill out the logicalResourceUses_
            for (auto access : task->logicalResourceAccesses_) {
                logicalResourceUses_[access.handle].push_back(TaskHandleLogicalResourceAccessPair{
                    .task = TaskHandle{.index = idx, .generation = dependencyGraph.tasks_[idx].generation},
                    .access = access.access,
                });
            }
        }

        handleExplicitDependencies(dependencyGraph);

        // Detect hazards
        // FIXME this part is essentially iterating in the task submission order. Should be order independent.
        // For now this is ok, but it means the responsibility of ordering the tasks of the graph is no in the
        // hands of the application.
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
            nodes_[bidx].incoming.push_back(aidx);
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
        // First of all, check if the dependency graph has a present set
        if (!dependencyGraph.isPresentSet_) {
            throw std::runtime_error("no present set for the graph, cannot compile");
        }

        for (std::uint32_t idx = 0; idx < dependencyGraph.logicalResources_.size(); idx++) {
            auto& logicalResourceIface = dependencyGraph.logicalResources_[idx].resource;

            CompiledLogicalResource compiledLogicalResource{};
            compiledLogicalResource.origin = LogicalResourceHandle{
                .index = idx, .generation = dependencyGraph.logicalResources_[idx].generation};

            // Set the initializer
            setInitializer(dependencyGraph, compiledLogicalResource);

            // If the resource is present resource, mark it
            if (compiledLogicalResource.origin == dependencyGraph.presentResourceHandle_) {
                compiledDependencyGraph_.presentResourceIdx = static_cast<std::int32_t>(idx);
            }

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
                    compiledLogicalResource.initStates.push_back(ResourceInitState::Uninitialized);
                    compiledLogicalResource.persistent = logicalImageResource->persistent;
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
                    compiledLogicalResource.initStates.push_back(ResourceInitState::Uninitialized);
                    compiledLogicalResource.persistent = logicalBufferResource->persistent;
                }
            }

            compiledDependencyGraph_.compiledLogicalResources.push_back(compiledLogicalResource);
        }
    }

    void DependencyGraphCompiler::setInitializer(
        DependencyGraph& dependencyGraph, CompiledLogicalResource& resource) {
        // Check if we have a user defined initializer, if not, add default one
        DependencyGraph::InitResourceInterface initIface = DependencyGraph::InitDefault{};
        if (dependencyGraph.resourceInits_.contains(resource.origin)) {
            initIface = dependencyGraph.resourceInits_[resource.origin];
        }

        resource.initContext = initIface;
    }

    core::ResourceHandle DependencyGraphCompiler::createPhysicalImageResource(
        LogicalImageResource* logicalImageResource) {
        return ctx_.resourceManager->createResource<core::Image>(core::ImageDesc{
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
        return ctx_.resourceManager->createResource<core::Buffer>(
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

    void DependencyGraphCompiler::compileTasks(DependencyGraph& dependencyGraph) {
        for (auto& level : compiledDependencyGraph_.executionLevels) {
            for (auto i : level) {
                BaseGpuTask* srcTask = nodes_[i].task;
                CompiledTask& dstTask = compiledDependencyGraph_.compiledTasks[i];

                // Origin
                dstTask.origin = TaskHandle{.index = i, .generation = dependencyGraph.tasks_[i].generation};

                // Task type and pipeline
                if (srcTask->getAs<ComputeTask>()) {
                    dstTask.type = CompiledTaskType::Compute;
                    // create compute pipeline
                    // TODO
                } else if (srcTask->getAs<GraphicsTask>()) {
                    dstTask.type = CompiledTaskType::Graphics;
                    // create graphics pipeline
                    // TODO
                }

                // Compile resource accesses
                compileTaskResourceAccesses(srcTask, dstTask);

                // Compile barriers
                for (auto p : nodes_[i].incoming) {
                    BaseGpuTask* prevTask = nodes_[p].task;
                    for (auto& prevAccess : prevTask->logicalResourceAccesses_) {
                        for (auto& currAccess : srcTask->logicalResourceAccesses_) {
                            if (prevAccess.handle != currAccess.handle) continue;

                            auto prevIntent = determineReadWriteIntent(prevAccess.access);
                            auto currIntent = determineReadWriteIntent(currAccess.access);

                            if (!isHazard(prevIntent, currIntent)) continue;

                            compileBarrier(prevAccess, currAccess, dstTask);
                        }
                    }
                }
            }
        }
    }

    void DependencyGraphCompiler::compileTaskResourceAccesses(BaseGpuTask* srcTask, CompiledTask& dstTask) {
        for (auto& access : srcTask->logicalResourceAccesses_) {
            CompiledLogicalResourceAccess compiledAccess{};

            // Map logical → compiled resource
            compiledAccess.compiledLogicalResource =
                &compiledDependencyGraph_.compiledLogicalResources[access.handle.index];

            // Binding info is copied verbatim
            compiledAccess.bindingIface = access.binding;

            // Descriptor type is decided here
            compiledAccess.descriptorType = determineDescriptorType(access.access);

            dstTask.compiledResourceAccesses.push_back(compiledAccess);
        }
    }

    vk::DescriptorType DependencyGraphCompiler::determineDescriptorType(AccessInterface access) {
        if (auto imageAccess = std::get_if<ImageAccess>(&access)) {
            switch (*imageAccess) {
                case ImageAccess::ColorAttachmentWrite:
                case ImageAccess::DepthAttachmentWrite:
                    return vk::DescriptorType::eInputAttachment;
                case ImageAccess::StorageReadWrite:
                    return vk::DescriptorType::eStorageImage;
                case ImageAccess::SampledRead:
                    return vk::DescriptorType::eCombinedImageSampler;
                default:
                    throw std::runtime_error("could not determine descriptor type for image");
            }
        }

        else if (auto bufferAccess = std::get_if<BufferAccess>(&access)) {
            switch (*bufferAccess) {
                case BufferAccess::UniformRead:
                    return vk::DescriptorType::eUniformBuffer;
                case BufferAccess::StorageReadWrite:
                    return vk::DescriptorType::eStorageBuffer;
                default:
                    throw std::runtime_error("could not determine descriptor type for buffer");
            }
        }

        throw std::runtime_error("could not determine descriptor type");
    }

    void DependencyGraphCompiler::compileBarrier(
        const LogicalResourceAccess& prev, const LogicalResourceAccess& curr, CompiledTask& dstTask) {
        vk::PipelineStageFlags2 srcStage = stagesFromAccess(prev.access);
        vk::PipelineStageFlags2 dstStage = stagesFromAccess(curr.access);
        vk::AccessFlags2 srcAccess = accessFlagsFromAccess(prev.access);
        vk::AccessFlags2 dstAccess = accessFlagsFromAccess(curr.access);

        if (auto oldAccess = std::get_if<ImageAccess>(&prev.access)) {
            if (auto newAccess = std::get_if<ImageAccess>(&curr.access)) {
                vk::ImageLayout oldLayout = layoutFromAccess(*oldAccess);
                vk::ImageLayout newLayout = layoutFromAccess(*newAccess);

                vk::ImageMemoryBarrier2 imageBarrier{
                    .srcStageMask = srcStage,
                    .srcAccessMask = srcAccess,
                    .dstStageMask = dstStage,
                    .dstAccessMask = dstAccess,
                    .oldLayout = oldLayout,
                    .newLayout = newLayout,
                };

                dstTask.imageBarriers.push_back(imageBarrier);
            }
        }

        if (auto oldAccess = std::get_if<BufferAccess>(&prev.access)) {
            if (auto newAccess = std::get_if<BufferAccess>(&curr.access)) {
                vk::BufferMemoryBarrier2 bufferBarrier{
                    .srcStageMask = srcStage,
                    .srcAccessMask = srcAccess,
                    .dstStageMask = dstStage,
                    .dstAccessMask = dstAccess,
                };

                dstTask.bufferBarriers.push_back(bufferBarrier);
            }
        }
    }

    vk::PipelineStageFlags2 DependencyGraphCompiler::stagesFromAccess(AccessInterface accessIface) {
        if (auto imageAccess = std::get_if<ImageAccess>(&accessIface)) {
            switch (*imageAccess) {
                case ImageAccess::ColorAttachmentWrite:
                    return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                case ImageAccess::DepthAttachmentWrite:
                    return vk::PipelineStageFlagBits2::eEarlyFragmentTests;
                case ImageAccess::StorageReadWrite:
                    return vk::PipelineStageFlagBits2::eComputeShader;
                case ImageAccess::SampledRead:
                    return vk::PipelineStageFlagBits2::eFragmentShader;
                default:
                    throw std::runtime_error("could not determine stage");
            }
        }

        else if (auto bufferAccess = std::get_if<BufferAccess>(&accessIface)) {
            switch (*bufferAccess) {
                case BufferAccess::UniformRead:
                    return vk::PipelineStageFlagBits2::eAllGraphics;
                case BufferAccess::StorageReadWrite:
                    return vk::PipelineStageFlagBits2::eComputeShader;
                default:
                    throw std::runtime_error("could not determine stage");
            }
        }

        throw std::runtime_error("could not determine stage");
    }

    vk::AccessFlags2 DependencyGraphCompiler::accessFlagsFromAccess(AccessInterface accessIface) {
        if (auto imageAccess = std::get_if<ImageAccess>(&accessIface)) {
            switch (*imageAccess) {
                case ImageAccess::ColorAttachmentWrite:
                    return vk::AccessFlagBits2::eColorAttachmentWrite;
                case ImageAccess::DepthAttachmentWrite:
                    return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                case ImageAccess::StorageReadWrite:
                    return vk::AccessFlagBits2::eShaderRead;
                case ImageAccess::SampledRead:
                    return vk::AccessFlagBits2::eShaderSampledRead;
                default:
                    throw std::runtime_error("could not determine access");
            }
        }

        else if (auto bufferAccess = std::get_if<BufferAccess>(&accessIface)) {
            switch (*bufferAccess) {
                case BufferAccess::UniformRead:
                    return vk::AccessFlagBits2::eUniformRead;
                case BufferAccess::StorageReadWrite:
                    return vk::AccessFlagBits2::eShaderRead;
                default:
                    throw std::runtime_error("could not determine access");
            }
        }

        throw std::runtime_error("could not determine access");
    }

    vk::ImageLayout DependencyGraphCompiler::layoutFromAccess(ImageAccess access) {
        switch (access) {
            case ImageAccess::ColorAttachmentWrite:
                return vk::ImageLayout::eColorAttachmentOptimal;
            case ImageAccess::DepthAttachmentWrite:
                return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case ImageAccess::StorageReadWrite:
                return vk::ImageLayout::eGeneral;
            case ImageAccess::SampledRead:
                return vk::ImageLayout::eShaderReadOnlyOptimal;
            default:
                throw std::runtime_error("could not determine image layout");
        }
    }

    CompiledDependencyGraph DependencyGraphCompiler::compile(DependencyGraph& dependencyGraph) {
        // Graph Analysis
        buildGraphNodes(dependencyGraph);
        determineExecutionLevels();

        // Resource Compilation
        compileLogicalResources(dependencyGraph);

        // Task compilation
        compileTasks(dependencyGraph);

        // Return compiled graph
        return std::move(compiledDependencyGraph_);
    }

    void DependencyGraphCompiler::createComputePipeline(CompiledTask& task) {}

}  // namespace hammock::renderer
