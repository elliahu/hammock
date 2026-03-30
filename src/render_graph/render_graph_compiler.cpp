#include "render_graph_compiler.hpp"

#include <algorithm>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <variant>

#include "render_graph.hpp"
#include "render_pass.hpp"

namespace hammock::graph {

    // Construction / top-level compile

    RenderGraphCompiler::RenderGraphCompiler() {}

    CompiledRenderGraph RenderGraphCompiler::compile(RenderGraph& dg) {
        // Reset internal state so the compiler object can be reused
        nodes_.clear();
        resourceUses_.clear();
        result_ = {};

        // Call the declaration callbacks
        callDeclFuncs(dg);

        // Graph analysis
        buildNodes(dg);  // populate nodes_ & resourceUses_
        applyExplicitDependencies(dg);
        buildDataFlowEdges();  // order-independent hazard edges
        assertDebugEdges(dg);
        buildExecutionLevels();  // Kahn topological sort

        // Resource compilation
        compileResources(dg);

        // Task + barrier compilation
        compileRenderPasses(dg);

        return std::move(result_);
    }

    // Graph analysis

    void RenderGraphCompiler::buildNodes(RenderGraph& dg) {
        const auto count = static_cast<uint32_t>(dg.passes_.size());
        nodes_.resize(count);
        result_.passes.resize(count);

        for (uint32_t i = 0; i < count; ++i) {
            nodes_[i].pass = &dg.passes_[i].pass;

            RenderPassHandle handle{.index = i, .generation = dg.passes_[i].generation};
            if (handle == dg.root_) {
                result_.root = static_cast<int32_t>(i);
            }

            // Record every resource access for this task (un-ordered at this point)
            for (const auto& acc : nodes_[i].pass->logicalResourceAccesses_) {
                resourceUses_[acc.handle].push_back(ResourceUseEntry{.passIdx = i, .access = acc.access});
            }
        }
    }

    void RenderGraphCompiler::addEdge(uint32_t src, uint32_t dst) {
        if (std::ranges::contains(nodes_[src].outgoing, dst)) return;  // already exists
        nodes_[src].outgoing.push_back(dst);
        nodes_[dst].incoming.push_back(src);
        ++nodes_[dst].indegree;
    }

    void RenderGraphCompiler::applyExplicitDependencies(RenderGraph& dg) {
        for (const auto& dep : dg.explicitDependencies_) {
            if (!dg.isHandleValid(dep.srcPassHandle) || !dg.isHandleValid(dep.dstPassHandle)) {
                throw std::runtime_error(
                    "DependencyGraphCompiler: invalid task handle in explicit dependency");
            }
            if (dep.dependencyType == DependencyType::Execution) {
                addEdge(dep.srcPassHandle.index, dep.dstPassHandle.index);
            }
            // Debug edges are only asserted, not structural – handled in assertDebugEdges
        }
    }

    void RenderGraphCompiler::buildDataFlowEdges() {
        // For each logical resource, look at every pair of tasks that access it.
        // Insert an edge whenever the pair constitutes a hazard (write involved).
        //
        // This is O(accesses to power of 2) per resource but the number of accesses per resource
        // is small in practice.  The key property: we do NOT rely on the order in
        // which accesses appear in the resourceUses_ list (i.e. declaration order).
        for (auto& [handle, uses] : resourceUses_) {
            const auto n = uses.size();
            for (size_t a = 0; a < n; ++a) {
                for (size_t b = 0; b < n; ++b) {
                    if (a == b) continue;

                    const auto intentA = intentOf(uses[a].access);
                    const auto intentB = intentOf(uses[b].access);

                    // Only emit an A->B edge when A writes and B reads/writes.
                    // Read->Read is fine with no edge.
                    // For Write->Write we let the explicit dependencies resolve the
                    // order; if neither is ordered explicitly the graph is ambiguous
                    // and the topological sort will leave them at the same level
                    // (which the executor must handle).
                    if (intentA == ReadWriteIntent::Read)
                        continue;                               // A is a pure reader – no outgoing hazard
                    if (!isHazard(intentA, intentB)) continue;  // RR – skip

                    // intentA is Write or ReadWrite -> B must wait for A
                    addEdge(uses[a].passIdx, uses[b].passIdx);
                }
            }
        }
    }

    void RenderGraphCompiler::assertDebugEdges(RenderGraph& dg) const {
        for (const auto& dep : dg.explicitDependencies_) {
            if (dep.dependencyType != DependencyType::Debug) continue;
            const uint32_t src = dep.srcPassHandle.index;
            const uint32_t dst = dep.dstPassHandle.index;
            if (!std::ranges::contains(nodes_[src].outgoing, dst)) {
                throw std::runtime_error("DependencyGraphCompiler: expected debug edge not found");
            }
        }
    }

    void RenderGraphCompiler::buildExecutionLevels() {
        // Work on a copy of indegrees so the original nodes_ are preserved for
        // the barrier compilation step that follows.
        std::vector<uint32_t> indegree(nodes_.size());
        for (uint32_t i = 0; i < nodes_.size(); ++i) {
            indegree[i] = nodes_[i].indegree;
        }

        std::queue<uint32_t> ready;
        for (uint32_t i = 0; i < nodes_.size(); ++i) {
            if (indegree[i] == 0) ready.push(i);
        }

        while (!ready.empty()) {
            std::vector<uint32_t> level;
            const size_t levelSize = ready.size();

            for (size_t i = 0; i < levelSize; ++i) {
                const uint32_t t = ready.front();
                ready.pop();
                level.push_back(t);

                for (uint32_t next : nodes_[t].outgoing) {
                    if (--indegree[next] == 0) ready.push(next);
                }
            }
            result_.levels.push_back(std::move(level));
        }
    }

    // Resource compilation

    void RenderGraphCompiler::compileResources(RenderGraph& dg) {
        if (!dg.hasRoot_) {
            throw std::runtime_error("DependencyGraphCompiler: dependency graph has no root task");
        }

        result_.resources.reserve(dg.logicalResources_.size());

        for (uint32_t i = 0; i < dg.logicalResources_.size(); ++i) {
            auto& entry = dg.logicalResources_[i];
            auto& iface = entry.resource;

            CompiledLogicalResource compiled{};
            compiled.origin = LogicalResourceHandle{.index = i, .generation = entry.generation};
            compiled.resource = entry.resource; // We coppy the data here

            result_.resources.push_back(std::move(compiled));
        }
    }

    // Task & barrier compilation

    void RenderGraphCompiler::compileRenderPasses(RenderGraph& dg) {
        // Track which resources have already received their init barrier so we
        // only emit it once (for the first task to use them).
        std::unordered_set<LogicalResourceHandle, LogicalResourceHandleHash> initialised;

        for (auto& level : result_.levels) {
            for (uint32_t i : level) {
                RenderPass* src = nodes_[i].pass;
                CompiledRenderPass& dst = result_.passes[i];

                dst.origin = RenderPassHandle{.index = i, .generation = dg.passes_[i].generation};
                dst.execFunc = std::move(src->execFunc_);
                dst.family = src->getFamily();
                dst.dependencies = nodes_[i].incoming; // mark the dependencies

                for (const auto& acc : src->logicalResourceAccesses_) {
                    dst.resources.push_back(acc.handle.index);
                }

                // Init barriers (UNDEFINED -> first layout)
                for (const auto& currAcc : src->logicalResourceAccesses_) {
                    if (initialised.contains(currAcc.handle)) continue;
                    initialised.insert(currAcc.handle);
                    emitInitBarrier(currAcc, dst);
                }

                // Transition barriers between predecessor tasks
                for (uint32_t p : nodes_[i].incoming) {
                    RenderPass* prev = nodes_[p].pass;

                    for (const auto& prevAcc : prev->logicalResourceAccesses_) {
                        for (const auto& currAcc : src->logicalResourceAccesses_) {
                            if (prevAcc.handle != currAcc.handle) continue;

                            if (!isHazard(intentOf(prevAcc.access), intentOf(currAcc.access))) continue;

                            emitBarrier(prevAcc, currAcc, dst);
                        }
                    }
                }

                // Compile-time callbacks
                if (src->compFunc_) {
                    RenderPassComplContext ctx{};
                    src->compFunc_(ctx);
                }
            }
        }
    }

    void RenderGraphCompiler::emitBarrier(
        const LogicalResourceAccess& prev, const LogicalResourceAccess& curr, CompiledRenderPass& dst) {
        const vk::PipelineStageFlags2 srcStage = stageOf(prev.access);
        const vk::PipelineStageFlags2 dstStage = stageOf(curr.access);
        const vk::AccessFlags2 srcAccess = accessOf(prev.access);
        const vk::AccessFlags2 dstAccess = accessOf(curr.access);

        if (const auto* oldImg = std::get_if<ImageAccess>(&prev.access)) {
            if (const auto* newImg = std::get_if<ImageAccess>(&curr.access)) {
                dst.barriers.image.push_back(vk::ImageMemoryBarrier2{
                    .srcStageMask = srcStage,
                    .srcAccessMask = srcAccess,
                    .dstStageMask = dstStage,
                    .dstAccessMask = dstAccess,
                    .oldLayout = layoutOf(*oldImg),
                    .newLayout = layoutOf(*newImg),
                });
            }
        } else if (std::get_if<BufferAccess>(&prev.access)) {
            if (std::get_if<BufferAccess>(&curr.access)) {
                dst.barriers.buffer.push_back(vk::BufferMemoryBarrier2{
                    .srcStageMask = srcStage,
                    .srcAccessMask = srcAccess,
                    .dstStageMask = dstStage,
                    .dstAccessMask = dstAccess,
                });
            }
        }
    }

    void RenderGraphCompiler::emitInitBarrier(const LogicalResourceAccess& firstUse, CompiledRenderPass& dst) {
        const vk::PipelineStageFlags2 dstStage = stageOf(firstUse.access);
        const vk::AccessFlags2 dstAccess = accessOf(firstUse.access);

        if (const auto* imgAccess = std::get_if<ImageAccess>(&firstUse.access)) {
            // Transition from UNDEFINED – no src sync needed, nothing was written yet.
            dst.barriers.image.push_back(vk::ImageMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eNone,
                .srcAccessMask = vk::AccessFlagBits2::eNone,
                .dstStageMask = dstStage,
                .dstAccessMask = dstAccess,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = layoutOf(*imgAccess),
            });
        } else if (std::get_if<BufferAccess>(&firstUse.access)) {
            // Buffers don't have layouts but we still emit an acquire-style barrier
            // so the executor can optionally insert it (most drivers are fine without
            // this for device-local buffers, but it makes validation layers happy).
            dst.barriers.buffer.push_back(vk::BufferMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eNone,
                .srcAccessMask = vk::AccessFlagBits2::eNone,
                .dstStageMask = dstStage,
                .dstAccessMask = dstAccess,
            });
        }
    }

    // Access helpers

    RenderGraphCompiler::ReadWriteIntent RenderGraphCompiler::intentOf(AccessInterface a) {
        if (auto* img = std::get_if<ImageAccess>(&a)) {
            switch (*img) {
                case ImageAccess::SampledRead:
                    return ReadWriteIntent::Read;
                case ImageAccess::ColorAttachmentWrite:
                    return ReadWriteIntent::Write;
                case ImageAccess::DepthAttachmentWrite:
                    return ReadWriteIntent::Write;
                case ImageAccess::StorageReadWrite:
                    return ReadWriteIntent::ReadWrite;
                default:
                    throw std::runtime_error("intentOf: unhandled ImageAccess");
            }
        }
        if (auto* buf = std::get_if<BufferAccess>(&a)) {
            switch (*buf) {
                case BufferAccess::UniformRead:
                    return ReadWriteIntent::Read;
                case BufferAccess::StorageReadWrite:
                    return ReadWriteIntent::ReadWrite;
                default:
                    throw std::runtime_error("intentOf: unhandled BufferAccess");
            }
        }
        throw std::runtime_error("intentOf: unknown AccessInterface variant");
    }

    bool RenderGraphCompiler::isHazard(ReadWriteIntent a, ReadWriteIntent b) {
        // Read–Read is safe; everything else requires synchronisation.
        return !(a == ReadWriteIntent::Read && b == ReadWriteIntent::Read);
    }

    vk::PipelineStageFlags2 RenderGraphCompiler::stageOf(AccessInterface a) {
        if (auto* img = std::get_if<ImageAccess>(&a)) {
            switch (*img) {
                case ImageAccess::ColorAttachmentWrite:
                    return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                case ImageAccess::DepthAttachmentWrite:
                    return vk::PipelineStageFlagBits2::eEarlyFragmentTests;
                case ImageAccess::StorageReadWrite:
                    return vk::PipelineStageFlagBits2::eComputeShader;
                case ImageAccess::SampledRead:
                    return vk::PipelineStageFlagBits2::eFragmentShader;
                default:
                    throw std::runtime_error("stageOf: unhandled ImageAccess");
            }
        }
        if (auto* buf = std::get_if<BufferAccess>(&a)) {
            switch (*buf) {
                case BufferAccess::UniformRead:
                    return vk::PipelineStageFlagBits2::eAllGraphics;
                case BufferAccess::StorageReadWrite:
                    return vk::PipelineStageFlagBits2::eComputeShader;
                default:
                    throw std::runtime_error("stageOf: unhandled BufferAccess");
            }
        }
        throw std::runtime_error("stageOf: unknown AccessInterface variant");
    }

    vk::AccessFlags2 RenderGraphCompiler::accessOf(AccessInterface a) {
        if (auto* img = std::get_if<ImageAccess>(&a)) {
            switch (*img) {
                case ImageAccess::ColorAttachmentWrite:
                    return vk::AccessFlagBits2::eColorAttachmentWrite;
                case ImageAccess::DepthAttachmentWrite:
                    return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                case ImageAccess::StorageReadWrite:
                    return vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
                case ImageAccess::SampledRead:
                    return vk::AccessFlagBits2::eShaderSampledRead;
                default:
                    throw std::runtime_error("accessOf: unhandled ImageAccess");
            }
        }
        if (auto* buf = std::get_if<BufferAccess>(&a)) {
            switch (*buf) {
                case BufferAccess::UniformRead:
                    return vk::AccessFlagBits2::eUniformRead;
                case BufferAccess::StorageReadWrite:
                    return vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
                default:
                    throw std::runtime_error("accessOf: unhandled BufferAccess");
            }
        }
        throw std::runtime_error("accessOf: unknown AccessInterface variant");
    }

    vk::ImageLayout RenderGraphCompiler::layoutOf(ImageAccess a) {
        switch (a) {
            case ImageAccess::ColorAttachmentWrite:
                return vk::ImageLayout::eColorAttachmentOptimal;
            case ImageAccess::DepthAttachmentWrite:
                return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case ImageAccess::StorageReadWrite:
                return vk::ImageLayout::eGeneral;
            case ImageAccess::SampledRead:
                return vk::ImageLayout::eShaderReadOnlyOptimal;
            default:
                throw std::runtime_error("layoutOf: unhandled ImageAccess");
        }
    }

    void RenderGraphCompiler::callDeclFuncs(RenderGraph& dg) {
        for (auto& pass : dg.passes_) {
            RenderPassBuilder builder{};
            pass.pass.declFunc_(builder);
            pass.pass.logicalResourceAccesses_.swap(builder.logicalResourceAccesses_);
        }
    }
}  // namespace hammock::graph
