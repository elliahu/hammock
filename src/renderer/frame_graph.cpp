#include "frame_graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <variant>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"

hammock::renderer::FrameGraph::FrameGraph(FrameGraphDesc&& desc, CommandCache& commandCache,
    core::ResourceRef<core::Semaphore> graphicsTimeline, core::ResourceRef<core::Semaphore> computeTimeline,
    core::ResourceRef<core::Semaphore> transferTimeline)
    : desc_{std::move(desc)},
      commandCache_(commandCache),
      timeline_({graphicsTimeline, computeTimeline, transferTimeline}) {}

void hammock::renderer::FrameGraph::execute(uint32_t frameIndex) {
    // Reset pools
    commandCache_.resetPools(frameIndex);

    // For each batch determine its family and allocate command buffer
    for (auto& pass : desc_.passes) {
        // Determine the pass family
        // And get correct timeline semaphore for the queue
        core::CommandQueueFamily queue = core::CommandQueueFamily::Ignored;
        core::ResourceRef<core::Semaphore> signalTimeline;
        if (std::holds_alternative<GraphicsPassDescription>(pass)) {
            queue = core::CommandQueueFamily::Graphics;
            signalTimeline = timeline_.graphics;
        } else if (std::holds_alternative<ComputePassDescription>(pass)) {
            queue = core::CommandQueueFamily::Compute;
            signalTimeline = timeline_.compute;
        } else if (std::holds_alternative<TransferPassDescription>(pass)) {
            queue = core::CommandQueueFamily::Transfer;
            signalTimeline = timeline_.transfer;
        }

        // Sanity check
        if (queue == core::CommandQueueFamily::Ignored) {
            throw std::runtime_error("failed to determine pass family");
        }

        // Unsupported transfer
        // TODO
        if (queue == core::CommandQueueFamily::Transfer) {
            throw std::runtime_error("transfer passes not yet supported");
        }

        // Get the primary cmd buffer reference
        core::ResourceRef<core::CommandBuffer> commandBuffer =
            commandCache_.getCommandBuffer(queue, core::CommandBufferLevel::Primary, frameIndex, 0);

        // Begin command buffer
        commandBuffer->begin();

        // Add sync primitives and record
        std::visit(
            [&commandBuffer, &signalTimeline, this](auto& anyPass) {
                // Add wait semaphore if present
                for (auto& wait : anyPass.sync.waits) {
                    core::ResourceRef<core::Semaphore> waitTimeline;

                    // determine the queue on which we wait
                    if (wait.queue == core::CommandQueueFamily::Graphics) {
                        waitTimeline = timeline_.graphics;
                    } else if (wait.queue == core::CommandQueueFamily::Compute) {
                        waitTimeline = timeline_.compute;
                    } else if (wait.queue == core::CommandQueueFamily::Transfer) {
                        waitTimeline = timeline_.transfer;
                    } else
                        throw std::runtime_error("could not determine the wait queue");

                    commandBuffer->addWaitSemaphore(waitTimeline, wait.stage, wait.value);
                }

                // Add signal semaphore
                for (auto& signal : anyPass.sync.signals){
                    commandBuffer->addSignalSemaphore(signalTimeline, signal.value);
                }

                // Record
                using PassType = std::decay_t<decltype(anyPass)>;
                if constexpr (std::is_same_v<PassType, GraphicsPassDescription>) {
                    recordGraphicsPass(anyPass, commandBuffer);
                } else if constexpr (std::is_same_v<PassType, ComputePassDescription>) {
                    recordComputePass(anyPass, commandBuffer);
                }
            },
            pass);

        // Submit the command buffer
        commandBuffer->submit();
    }
}

void hammock::renderer::FrameGraph::recordBarrier(
    PassTransitionsInfo& transitions, core::ResourceRef<core::CommandBuffer> cmd) {
    if (!transitions.images.empty() || !transitions.buffers.empty()) {
        std::vector<core::ImageMemoryBarrier> imageBarriers{};
        std::vector<core::BufferMemoryBarrier> bufferBarriers{};

        for (auto& imageTransition : transitions.images) {
            imageBarriers.push_back(imageTransition);
        }

        for (auto& bufferTransition : transitions.buffers) {
            bufferBarriers.push_back(bufferTransition);
        }

        cmd->pipelineBarrier(imageBarriers, bufferBarriers);
    }
}

void hammock::renderer::FrameGraph::recordGraphicsPass(
    GraphicsPassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd) {
    // Barriers first
    recordBarrier(pass.transitions, cmd);

    bool beginRendering =
        !pass.rendering.colorAttachments.empty() || pass.rendering.depthAttachment.has_value();
    bool setViewport = pass.rendering.viewport.has_value();
    bool setScissor = pass.rendering.scissors.has_value();
    bool bindPipeline = pass.rendering.pipeline.has_value();
    bool bindDescriptors = !pass.data.descriptors.empty();
    bool pushConstants = pass.data.constants.data != nullptr && pass.data.constants.size >= 0;

    // Automatically begin rendering
    if (beginRendering) {
        // If render are is not set, color first attachment is used
        auto renderArea = pass.rendering.area.value_or(
            pass.rendering.colorAttachments.empty() ? pass.rendering.depthAttachment.value()->createScissor()
                                                    : pass.rendering.colorAttachments[0]->createScissor());
        cmd->beginRendering(renderArea,
            pass.rendering.colorAttachments,
            pass.rendering.depthAttachment,
            pass.rendering.stencilAttachment);
    }

    // Bind pipeline
    if (bindPipeline) {
        cmd->bindPipeline(pass.rendering.pipeline.value());
    }

    // Set viewport
    if (setViewport) {
        cmd->setViewport(pass.rendering.viewport.value());
    }

    // Set scissor
    if (setScissor) {
        cmd->setScissor(pass.rendering.scissors.value());
    }

    // Bind descriptors
    if (bindDescriptors) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot bind descriptors");

        cmd->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pass.rendering.pipeline.value(), pass.data.descriptors);
    }

    // Push constants
    if (pushConstants) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot push constants");

        cmd->pushConstants(pass.rendering.pipeline.value(),
            pass.data.constants.stages,
            0,
            pass.data.constants.size,
            pass.data.constants.data);
    }

    // Execute custom code
    if (pass.execute) {
        pass.execute(cmd);
    }

    // Automatically end rendering
    if (beginRendering) {
        cmd->endRendering();
    }
}

void hammock::renderer::FrameGraph::recordComputePass(
    ComputePassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd) {
    // Barriers first
    recordBarrier(pass.transitions, cmd);

    bool dispatch = pass.dispatch.sizes.x > 0 && pass.dispatch.sizes.y > 0 && pass.dispatch.sizes.z > 0;
    bool bindPipeline = pass.dispatch.pipeline.has_value();
    bool bindDescriptors = !pass.data.descriptors.empty();
    bool pushConstants = pass.data.constants.data != nullptr && pass.data.constants.size >= 0;

    // Bind pipeline
    if (bindPipeline) {
        cmd->bindPipeline(pass.dispatch.pipeline.value());
    }

    // Bind descriptors
    if (bindDescriptors) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot bind descriptors");

        cmd->bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, pass.dispatch.pipeline.value(), pass.data.descriptors);
    }

    // Push constants
    if (pushConstants) {
        if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot push constants");

        cmd->pushConstants(pass.dispatch.pipeline.value(),
            pass.data.constants.stages,
            0,
            pass.data.constants.size,
            pass.data.constants.data);
    }

    // Dispatch
    if (dispatch) {
        cmd->dispatch(pass.dispatch.sizes.x, pass.dispatch.sizes.y, pass.dispatch.sizes.z);
    }

    // Execute custom code
    if (pass.execute) {
        pass.execute(cmd);
    }
}

hammock::core::Handle<hammock::renderer::FrameGraphPassDeclarationWrapper>
hammock::renderer::FrameGraphBuilder::createGraphicsPass(GraphicsPassDeclaration&& decl) {
    auto handle = declarations_.create(std::move(decl));
    handles_.push_back(handle);
    return handle;
}

hammock::core::Handle<hammock::renderer::FrameGraphPassDeclarationWrapper>
hammock::renderer::FrameGraphBuilder::createComputePass(ComputePassDeclaration&& decl) {
    auto handle = declarations_.create(std::move(decl));
    handles_.push_back(handle);
    return handle;
}

hammock::renderer::FrameGraphDesc hammock::renderer::FrameGraphBuilder::build() {
    // Topologicaly sort the graph
    std::vector<core::Handle<FrameGraphPassDeclarationWrapper>> sorted;
    performTopoSort(sorted);

    // Create the output struct
    FrameGraphDesc desc{};

    //
    for (auto& handle : sorted) {
        core::ResourceRef<FrameGraphPassDeclarationWrapper> declVariant = declarations_.ref(handle);
        std::visit(
            [&desc, this](auto& anyDecl) {
                using DeclarationType = std::decay_t<decltype(anyDecl)>;
                if constexpr (std::is_same_v<DeclarationType, GraphicsPassDeclaration>) {
                    auto description = buildGraphicsPassDescription(anyDecl);

                    desc.passes.push_back(description);
                } else if constexpr (std::is_same_v<DeclarationType, ComputePassDeclaration>) {
                    auto description = buildComputePassDescription(anyDecl);

                    desc.passes.push_back(description);
                }
            },
            declVariant->declaration);
    }

    // Build the sync values
    buildSyncValues(desc, sorted);

    return std::move(desc);
}

hammock::renderer::GraphicsPassDescription hammock::renderer::FrameGraphBuilder::buildGraphicsPassDescription(
    GraphicsPassDeclaration& decl) {
    return GraphicsPassDescription{
        .name = std::move(decl.name),
        .transitions = deduceFromState(decl.resources, decl.dependency),
        .rendering = std::move(decl.rendering),
        .data = std::move(decl.data),
    };
}

hammock::renderer::ComputePassDescription hammock::renderer::FrameGraphBuilder::buildComputePassDescription(
    ComputePassDeclaration& decl) {
    return ComputePassDescription{
        .name = std::move(decl.name),
        .transitions = deduceFromState(decl.resources, decl.dependency),
        .dispatch = std::move(decl.dispatch),
        .data = std::move(decl.data),
    };
}

void hammock::renderer::FrameGraphBuilder::buildSyncValues(
    FrameGraphDesc& desc, std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>& sorted) {
    std::unordered_map<core::CommandQueueFamily, uint32_t> queueCounters{
        {core::CommandQueueFamily::Graphics, 0},
        {core::CommandQueueFamily::Compute, 0},
        {core::CommandQueueFamily::Transfer, 0},
    };

    for (int i = 0; i < desc.passes.size(); i++) {
        PassSyncInfo syncInfo{};
        std::variant<GraphicsPassDescription, ComputePassDescription, TransferPassDescription>& p =
            desc.passes[i];
        std::variant<GraphicsPassDeclaration, ComputePassDeclaration, TransferPassDeclaration>& decl =
            declarations_.ref(sorted[i])->declaration;

        std::visit(
            [this, &syncInfo, &decl, &queueCounters, &desc, &sorted](auto& pass) {
                using PassType = std::decay_t<decltype(pass)>;

                // Gather waits
                std::visit(
                    [this, &syncInfo, &desc, &sorted](auto& declaration) {
                        for (auto& depHandle : declaration.dependency.passes) {
                            // Find the dependency's index in the sorted list
                            auto depIt = std::find(sorted.begin(), sorted.end(), depHandle);
                            if (depIt == sorted.end()) continue;
                            int depIdx = std::distance(sorted.begin(), depIt);

                            // Read sync info already assigned to the dependency (topo order guarantees this)
                            /*std::visit(
                                [&syncInfo](auto& depPass) {
                                    syncInfo.waits.push_back({.value = depPass.sync.signal.value,
                                        .queue = depPass.sync.signal.queue,
                                        .stage = core::PipelineStage::TopOfPipeline});
                                },
                                desc.passes[depIdx]);*/
                        }
                    },
                    decl);

                core::CommandQueueFamily family = getQueueFamily<PassType>();

                if (family == core::CommandQueueFamily::Ignored) {
                    throw std::runtime_error("could not determine pass family");
                }

                queueCounters[family] += 1;
             /*  syncInfo.signal.queue = family;
                syncInfo.signal.value = queueCounters[family];
                syncInfo.signal.stage = core::PipelineStage::BottomOfPipeline;*/

                pass.sync = std::move(syncInfo);
            },
            p);
    }
}

hammock::renderer::PassTransitionsInfo hammock::renderer::FrameGraphBuilder::deduceFromState(
    PassResourceAccessInfo& accesses, PassDependencyInfo& dependencies) {
    PassTransitionsInfo transitions{};
    // For each image used in the current pass, find the state in which dependency pass leaves it
    for (auto& access : accesses.images) {
        core::ResourceState previousState = core::ResourceState::Undefined;
        auto it = std::find_if(dependencies.passes.begin(),
            dependencies.passes.end(),
            [this, &access, &previousState](auto& passWrapperHandle) {
                auto passWrapperRef = declarations_.ref(passWrapperHandle);

                return std::visit(
                    [&access, &previousState](auto& declaration) {
                        auto im = std::find_if(declaration.resources.images.begin(),
                            declaration.resources.images.end(),
                            [&access, &previousState](auto& image) { return image.image == access.image; });
                        if (im == declaration.resources.images.end()) {
                            return false;
                        }
                        previousState = (*im).state;
                        return im != declaration.resources.images.end();
                    },
                    passWrapperRef->declaration);
            });

        transitions.images.push_back({.image = access.image, .from = previousState, .to = access.state});
    }

    // Same for buffers
    for (auto& access : accesses.buffers) {
        core::ResourceState previousState = core::ResourceState::Undefined;
        auto it = std::find_if(dependencies.passes.begin(),
            dependencies.passes.end(),
            [this, &access, &previousState](auto& passWrapperHandle) {
                auto passWrapperRef = declarations_.ref(passWrapperHandle);

                return std::visit(
                    [&access, &previousState](auto& declaration) {
                        auto buff = std::find_if(declaration.resources.buffers.begin(),
                            declaration.resources.buffers.end(),
                            [&access, &previousState](
                                auto& buffer) { return buffer.buffer == access.buffer; });
                        if (buff == declaration.resources.buffers.end()) {
                            return false;
                        }
                        previousState = (*buff).state;
                        return buff != declaration.resources.buffers.end();
                    },
                    passWrapperRef->declaration);
            });

        transitions.buffers.push_back({.buffer = access.buffer, .from = previousState, .to = access.state});
    }
    return transitions;
}

void hammock::renderer::FrameGraphBuilder::dfs(core::Handle<FrameGraphPassDeclarationWrapper> node,
    std::unordered_map<core::Handle<FrameGraphPassDeclarationWrapper>, NodeVisitState>& state,
    std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>& sorted) {
    if (state[node] == NodeVisitState::Visiting) {
        throw std::runtime_error("cycle in frame graph detected");
    }

    if (state[node] == NodeVisitState::Visited) {
        return;
    }

    state[node] = NodeVisitState::Visiting;
    auto nodeRef = declarations_.ref(node);

    // Visit all dependencies first
    std::visit(
        [this, &state, &sorted](auto& declaration) {
            for (core::Handle<FrameGraphPassDeclarationWrapper>& dep : declaration.dependency.passes) {
                dfs(dep, state, sorted);
            }
        },
        nodeRef->declaration);

    state[node] = NodeVisitState::Visited;

    sorted.push_back(node);
}

void hammock::renderer::FrameGraphBuilder::performTopoSort(
    std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>& sorted) {
    std::unordered_map<core::Handle<FrameGraphPassDeclarationWrapper>, NodeVisitState> state{};

    for (auto& node : handles_) {
        state[node] = NodeVisitState::Unvisited;
    }

    for (auto& node : handles_) {
        if (state[node] == NodeVisitState::Unvisited) {
            dfs(node, state, sorted);
        }
    }
}
