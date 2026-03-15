#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "framebuffer.hpp"

hammock::renderer::Framebuffer::Framebuffer(core::Device &device, std::uint32_t framesInFlight, math::Vec2 resolution,
                                          core::ImageFormat format) : device_(device), resolution_(resolution),
                                                               framesInFlight_(framesInFlight){
    createImages(resolution, format);
}


hammock::renderer::Framebuffer::~Framebuffer() {
    for (auto &i: handles_) {
        images_.destroy(i);
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

hammock::core::ResourceRef<hammock::core::Image> hammock::renderer::Framebuffer::getFrontbufferImage(){
    return images_.ref(handles_[currentFrame_]);
}


void hammock::renderer::Framebuffer::createImages(math::Vec2 resolution, core::ImageFormat format) {
    for (int i = 0; i < framesInFlight_; i++) {
        auto handle = images_.create(device_, core::ImageDesc{
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
        handles_.push_back(handle);
    }

    // Wait for all transitions
    device_.waitIdle();
}
