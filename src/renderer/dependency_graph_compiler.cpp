module;

#include <cstdint>
#include <vector>
#include <variant>
#include <stdexcept>
#include <memory>


module hammock_renderer;

import hammock_core;
import :dependency_graph;

namespace hammock::renderer {
    void DependencyGraphCompiler::compileLogicalResources(DependencyGraph &dependencyGraph) {
        for (std::uint32_t idx = 0; idx < dependencyGraph.logicalResources_.size(); idx++) {
            auto &resources = core::ResourceManager::getInstance();
            auto &logicalResourceSlot = dependencyGraph.logicalResources_[idx];
            auto &logicalResourceInterface = logicalResourceSlot.resource;
            auto compiledResource = CompiledLogicalResource{
                .origin = {.index = idx, .generation = logicalResourceSlot.generation},
            };

            // Logical resource is an image
            if (auto logicalImage = std::get_if<LogicalImageResource>(&logicalResourceInterface)) {
                if (logicalImage->width == 0 || logicalImage->height == 0 || logicalImage->depth == 0 || logicalImage->
                    channels == 0 || logicalImage->channels > 4) {
                    throw std::runtime_error("invalid image extent");
                }

                // Create the description
                auto desc = core::ImageDesc{
                    .width = logicalImage->width,
                    .height = logicalImage->height,
                    .channels = logicalImage->channels,
                    .depth = logicalImage->depth,
                    .layers = logicalImage->layers,
                    .format = logicalImage->format,
                    .usage = logicalImage->usage,
                    .type = logicalImage->type,
                };

                auto numberOfCopies = 1u;
                if (logicalImage->frameLocal) {
                    numberOfCopies = core::SwapChain::MAX_FRAMES_IN_FLIGHT;
                }

                for (int i = 0; i < numberOfCopies; i++) {
                    auto handle = resources.createResource<core::Image>(desc);
                    compiledResource.handles.push_back(handle);
                }
            }

            // logical resource is a buffer
            if (auto logicalBuffer = std::get_if<LogicalBufferResource>(&logicalResourceInterface)) {
                auto desc = core::BufferDesc{
                    .type = logicalBuffer->type,
                    .usage = logicalBuffer->usage,
                    .instanceSize = logicalBuffer->instanceSize,
                    .instanceCount = logicalBuffer->instanceCount,
                };

                auto numberOfCopies = 1u;
                if (logicalBuffer->frameLocal) {
                    numberOfCopies = core::SwapChain::MAX_FRAMES_IN_FLIGHT;
                }

                for (int i = 0; i < numberOfCopies; i++) {
                    auto handle = resources.createResource<core::Buffer>(desc);
                    compiledResource.handles.push_back(handle);
                }
            }

            compiledDependencyGraph_.compiledResources.push_back(compiledResource);
        }
    }
}
