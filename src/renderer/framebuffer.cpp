#include <cstdint>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "framebuffer.hpp"
#include "core/vulkan_context.hpp"

hammock::renderer::Framebuffer::Framebuffer(core::VulkanContext &ctx, std::uint32_t framesInFlight, math::Vec2 resolution,
                                          core::ImageFormat format) : ctx_(ctx), resolution_(resolution),
                                                               framesInFlight_(framesInFlight) {
    createImages(resolution, format);
}

hammock::renderer::Framebuffer::~Framebuffer() {
    for (auto &i: images_) {
        ctx_.resourceManager->releaseResource(i.getUid());
    }
}

vk::Extent2D hammock::renderer::Framebuffer::getExtent() const {
    return vk::Extent2D(
        static_cast<std::uint32_t>(resolution_.X),
        static_cast<std::uint32_t>(resolution_.Y)
    );
}

void hammock::renderer::Framebuffer::swapImages() {
    currentFrame_ = (currentFrame_ + 1) % framesInFlight_;
}

hammock::core::ResourceHandle hammock::renderer::Framebuffer::getFrontbufferImage() const {
    return images_[currentFrame_];
}


void hammock::renderer::Framebuffer::createImages(math::Vec2 resolution, core::ImageFormat format) {
    for (int i = 0; i < framesInFlight_; i++) {
        auto handle = ctx_.resourceManager->createResource<core::Image>(core::ImageDesc{
            .width = static_cast<std::uint32_t>(resolution.X),
            .height = static_cast<std::uint32_t>(resolution.Y),
            .channels = 4,
            .depth = 1,
            .layers = 1,
            .mips = 1,
            .format = format,
            .usage = core::ImageUsage::ColorAttachment | core::ImageUsage::TransferSrc | core::ImageUsage::TransferDst,
            .type = core::ImageType::Type2D,
        });
        images_.push_back(handle);
    }

    // Wait for all transitions
    ctx_.device->waitIdle();
}
