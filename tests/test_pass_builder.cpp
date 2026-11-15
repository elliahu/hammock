#include <gtest/gtest.h>

#include "pass_builder.hpp"

using namespace hammock;

TEST(PassBuilderTests, BuildsComputePassDescription) {
    uint32_t constants = 42;
    auto pass = renderer::PassBuilder(core::CommandQueueFamily::Compute)
                    .transitionImage({}, core::ResourceState::Undefined, core::ResourceState::StorageImage)
                    .addDescriptor(core::DescriptorSet{})
                    .pushConstants(core::ShaderStage::Compute, sizeof(constants), &constants)
                    .signal(7, core::CommandQueueFamily::Compute, core::PipelineStage::ComputeShader)
                    .setDispatch(renderer::DispatchDescription{ .sizes = { .x = 2, .y = 3, .z = 1 } })
                    .build();

    EXPECT_EQ(pass.family, core::CommandQueueFamily::Compute);
    ASSERT_EQ(pass.transitions.images.size(), 1);
    EXPECT_EQ(pass.transitions.images[0].from, core::ResourceState::Undefined);
    EXPECT_EQ(pass.transitions.images[0].to, core::ResourceState::StorageImage);
    ASSERT_EQ(pass.data.descriptors.size(), 1);
    EXPECT_EQ(pass.data.constants.stages, core::ShaderStage::Compute);
    EXPECT_EQ(pass.data.constants.size, sizeof(constants));
    EXPECT_EQ(pass.data.constants.data, &constants);
    ASSERT_EQ(pass.sync.signals.size(), 1);
    EXPECT_EQ(pass.sync.signals[0].value, 7);
    EXPECT_EQ(pass.dispatch.sizes.x, 2);
    EXPECT_EQ(pass.dispatch.sizes.y, 3);
    EXPECT_EQ(pass.dispatch.sizes.z, 1);
}

TEST(PassBuilderTests, BuildsGraphicsPassDescription) {
    auto pass = renderer::PassBuilder(core::CommandQueueFamily::Graphics)
                    .transitionBuffer({}, core::ResourceState::TransferDst, core::ResourceState::VertexBuffer)
                    .waitFor(5, core::CommandQueueFamily::Compute, core::PipelineStage::TopOfPipeline)
                    .setRendering(renderer::RenderingDescription{
                        .colorAttachments = { core::ResourceRef<core::Image>{} },
                    })
                    .build();

    EXPECT_EQ(pass.family, core::CommandQueueFamily::Graphics);
    ASSERT_EQ(pass.transitions.buffers.size(), 1);
    EXPECT_EQ(pass.transitions.buffers[0].from, core::ResourceState::TransferDst);
    EXPECT_EQ(pass.transitions.buffers[0].to, core::ResourceState::VertexBuffer);
    ASSERT_EQ(pass.sync.waits.size(), 1);
    EXPECT_EQ(pass.sync.waits[0].value, 5);
    ASSERT_EQ(pass.rendering.colorAttachments.size(), 1);
}
