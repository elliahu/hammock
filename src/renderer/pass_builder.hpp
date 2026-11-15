#pragma once

#include <cstdint>
#include <functional>

#include "frame_graph.hpp"

namespace hammock::renderer {

    class PassBuilder {
       public:
        explicit PassBuilder(core::CommandQueueFamily family);

        PassBuilder& transitionImage(
            core::ResourceRef<core::Image> image, core::ResourceState before, core::ResourceState after);
        PassBuilder& transitionBuffer(
            core::ResourceRef<core::Buffer> buffer, core::ResourceState before, core::ResourceState after);
        PassBuilder& addDescriptor(core::DescriptorSet descriptor);
        PassBuilder& pushConstants(core::ShaderStage stages, uint32_t size, const void* data);
        PassBuilder& waitFor(uint64_t value, core::CommandQueueFamily queue, core::PipelineStage stage);
        PassBuilder& signal(uint64_t value, core::CommandQueueFamily queue, core::PipelineStage stage);
        PassBuilder& setRendering(RenderingDescription rendering);
        PassBuilder& setDispatch(DispatchDescription dispatch);
        PassBuilder& setExecute(std::function<void(core::ResourceRef<core::CommandBuffer>)> execute);

        [[nodiscard]] PassDescription build();

       private:
        PassDescription description_{};
    };

}  // namespace hammock::renderer
