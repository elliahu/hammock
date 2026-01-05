module;
#include <cstdint>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_engine.framebuffer;

hammock::engine::Framebuffer::Framebuffer(core::Device &device, std::uint32_t framesInFlight, math::Vec2 resolution,
                                          vk::Format format) : device_(device), resolution_(resolution),
                                                               framesInFlight_(framesInFlight) {
    createImages(resolution, format);
}

hammock::engine::Framebuffer::~Framebuffer() {
    auto &rm = core::ResourceManager::getInstance();
    for (auto &i: images_) {
        rm.releaseResource(i.getUid());
    }
}

vk::Extent2D hammock::engine::Framebuffer::getExtent() const {
    return vk::Extent2D(
        static_cast<std::uint32_t>(resolution_.X),
        static_cast<std::uint32_t>(resolution_.Y)
    );
}

void hammock::engine::Framebuffer::swapImages() {
    currentFrame_ = (currentFrame_ + 1) % framesInFlight_;
}

hammock::core::ResourceHandle hammock::engine::Framebuffer::getFrontbufferImage() const {
    return images_[currentFrame_];
}


void hammock::engine::Framebuffer::createImages(math::Vec2 resolution, vk::Format format) {
    auto &rm = core::ResourceManager::getInstance();
    for (int i = 0; i < framesInFlight_; i++) {
        auto imageDesc = core::ImageDesc{
            .width = static_cast<std::uint32_t>(resolution.X),
            .height = static_cast<std::uint32_t>(resolution.Y),
            .channels = 4,
            .depth = 1,
            .layers = 1,
            .mips = 1,
            .format = format,
            .usage = vk::ImageUsageFlagBits::eColorAttachment |
                     vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eTransferDst,
            .imageType = vk::ImageType::e2D,
            .imageViewType = vk::ImageViewType::e2D,
            .clearValue = vk::ClearValue(
                vk::ClearColorValue{
                    std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}
                }
            )
        };
        auto handle = rm.createResource<core::Image>("swapimage-" + std::to_string(i), imageDesc);
        images_.push_back(handle);
    }

    // Wait for all transitions
    device_.waitIdle();
}
