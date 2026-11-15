#include "pass_recorder.hpp"

#include <stdexcept>

namespace hammock::renderer {

    void PassRecorder::record(
        PassDescription& pass, core::ResourceRef<core::CommandBuffer> commandBuffer) const {
        if (pass.family == core::CommandQueueFamily::Graphics) {
            recordGraphicsPass(pass, commandBuffer);
        } else if (pass.family == core::CommandQueueFamily::Compute) {
            recordComputePass(pass, commandBuffer);
        } else {
            throw std::runtime_error("unsupported pass family for command recording");
        }
    }

    void PassRecorder::recordGraphicsPass(
        PassDescription& pass, core::ResourceRef<core::CommandBuffer> commandBuffer) const {
        bool beginRendering =
            !pass.rendering.colorAttachments.empty() || pass.rendering.depthAttachment.has_value();
        bool bindPipeline = pass.rendering.pipeline.has_value();
        bool bindDescriptors = !pass.data.descriptors.empty();
        bool pushConstants = pass.data.constants.data != nullptr && pass.data.constants.size > 0;

        if (beginRendering) {
            auto renderArea =
                pass.rendering.area.value_or(pass.rendering.colorAttachments.empty()
                                                 ? pass.rendering.depthAttachment.value()->createScissor()
                                                 : pass.rendering.colorAttachments[0]->createScissor());
            commandBuffer->beginRendering(renderArea,
                pass.rendering.colorAttachments,
                pass.rendering.depthAttachment,
                pass.rendering.stencilAttachment);
        }

        if (bindPipeline) {
            commandBuffer->bindPipeline(pass.rendering.pipeline.value());
        }

        if (pass.rendering.viewport) {
            commandBuffer->setViewport(pass.rendering.viewport.value());
        }

        if (pass.rendering.scissors) {
            commandBuffer->setScissor(pass.rendering.scissors.value());
        }

        if (bindDescriptors) {
            if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot bind descriptors");

            commandBuffer->bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, pass.rendering.pipeline.value(), pass.data.descriptors);
        }

        if (pushConstants) {
            if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot push constants");

            commandBuffer->pushConstants(pass.rendering.pipeline.value(),
                pass.data.constants.stages,
                0,
                pass.data.constants.size,
                pass.data.constants.data);
        }

        if (pass.execute) {
            pass.execute(commandBuffer);
        }

        if (beginRendering) {
            commandBuffer->endRendering();
        }
    }

    void PassRecorder::recordComputePass(
        PassDescription& pass, core::ResourceRef<core::CommandBuffer> commandBuffer) const {
        bool dispatch = pass.dispatch.sizes.x > 0 && pass.dispatch.sizes.y > 0 && pass.dispatch.sizes.z > 0;
        bool bindPipeline = pass.dispatch.pipeline.has_value();
        bool bindDescriptors = !pass.data.descriptors.empty();
        bool pushConstants = pass.data.constants.data != nullptr && pass.data.constants.size > 0;

        if (bindPipeline) {
            commandBuffer->bindPipeline(pass.dispatch.pipeline.value());
        }

        if (bindDescriptors) {
            if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot bind descriptors");

            commandBuffer->bindDescriptorSets(
                vk::PipelineBindPoint::eCompute, pass.dispatch.pipeline.value(), pass.data.descriptors);
        }

        if (pushConstants) {
            if (!bindPipeline) throw std::invalid_argument("pipeline was not set. cannot push constants");

            commandBuffer->pushConstants(pass.dispatch.pipeline.value(),
                pass.data.constants.stages,
                0,
                pass.data.constants.size,
                pass.data.constants.data);
        }

        if (dispatch) {
            commandBuffer->dispatch(pass.dispatch.sizes.x, pass.dispatch.sizes.y, pass.dispatch.sizes.z);
        }

        if (pass.execute) {
            pass.execute(commandBuffer);
        }
    }

}  // namespace hammock::renderer
