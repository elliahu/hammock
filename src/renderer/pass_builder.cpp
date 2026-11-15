#include "pass_builder.hpp"

#include <utility>

namespace hammock::renderer {

    PassBuilder::PassBuilder(core::CommandQueueFamily family) { description_.family = family; }

    PassBuilder& PassBuilder::transitionImage(
        core::ResourceRef<core::Image> image, core::ResourceState before, core::ResourceState after) {
        description_.transitions.images.push_back({ std::move(image), before, after });
        return *this;
    }

    PassBuilder& PassBuilder::transitionBuffer(
        core::ResourceRef<core::Buffer> buffer, core::ResourceState before, core::ResourceState after) {
        description_.transitions.buffers.push_back({ std::move(buffer), before, after });
        return *this;
    }

    PassBuilder& PassBuilder::addDescriptor(core::DescriptorSet descriptor) {
        description_.data.descriptors.push_back(descriptor);
        return *this;
    }

    PassBuilder& PassBuilder::pushConstants(core::ShaderStage stages, uint32_t size, const void* data) {
        description_.data.constants = { stages, size, data };
        return *this;
    }

    PassBuilder& PassBuilder::waitFor(
        uint64_t value, core::CommandQueueFamily queue, core::PipelineStage stage) {
        description_.sync.waits.push_back({ value, queue, stage });
        return *this;
    }

    PassBuilder& PassBuilder::signal(
        uint64_t value, core::CommandQueueFamily queue, core::PipelineStage stage) {
        description_.sync.signals.push_back({ value, queue, stage });
        return *this;
    }

    PassBuilder& PassBuilder::setRendering(RenderingDescription rendering) {
        description_.rendering = std::move(rendering);
        return *this;
    }

    PassBuilder& PassBuilder::setDispatch(DispatchDescription dispatch) {
        description_.dispatch = std::move(dispatch);
        return *this;
    }

    PassBuilder& PassBuilder::setExecute(
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute) {
        description_.execute = std::move(execute);
        return *this;
    }

    PassDescription PassBuilder::build() { return std::move(description_); }

}  // namespace hammock::renderer
