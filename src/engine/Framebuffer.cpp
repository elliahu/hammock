module;
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.engine.framebuffer;

hammock::engine::Framebuffer::Framebuffer(core::Device &device, std::uint32_t framesInFlight, math::Vec2 resolution,
                                          vk::Format format) : device_(device), resolution_(resolution),
                                                               framesInFlight_(
                                                                   framesInFlight) {
    createImages(resolution, format);
    createCommandBuffers();
    createSemaphores();
}

hammock::engine::Framebuffer::~Framebuffer() {
    auto &rm = core::ResourceManager::getInstance();
    for (auto &i: images_) {
        rm.releaseResource(i.getUid());
    }
    commandBuffers_.clear();
    framebufferReadySemaphores_.clear();
}

vk::Extent2D hammock::engine::Framebuffer::getExtent() const {
    return vk::Extent2D{
        .width = static_cast<std::uint32_t>(resolution_.X),
        .height = static_cast<std::uint32_t>(resolution_.Y),
    };
}

void hammock::engine::Framebuffer::swapImages() {
    currentFrame_ = (currentFrame_ + 1) % framesInFlight_;

    auto image = core::ResourceManager::getInstance().getResource<core::Image>(getFrontbufferImage());
    auto &commandBuffer = getFrontBufferCommandBuffer();
    commandBuffer.signalSemaphore(*framebufferReadySemaphores_[currentFrame_]);

    commandBuffer.begin();

    image->recordPipelineBarrier(
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );

    commandBuffer.submit();
}

hammock::core::ResourceHandle hammock::engine::Framebuffer::getFrontbufferImage() const {
    return getImage(currentFrame_);
}

hammock::core::ResourceHandle hammock::engine::Framebuffer::getImage(std::uint32_t index) const {
    if (index >= images_.size()) {
        throw std::runtime_error("Invalid framebuffer image index");
    }
    return images_[index];
}

hammock::core::Image *hammock::engine::Framebuffer::getImagePtr(std::uint32_t index) const {
    if (index >= images_.size()) {
        throw std::runtime_error("Invalid framebuffer image index");
    }
    return core::ResourceManager::getInstance().getResource<core::Image>(images_[index]);
}

hammock::core::Semaphore &hammock::engine::Framebuffer::getFramebufferReadySemaphore() const {
    return *framebufferReadySemaphores_[currentFrame_];
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
            .clearValue = vk::ClearValue{
                .color = vk::ClearColorValue{
                    std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}
                }
            }
        };
        auto handle = rm.createResource<core::Image>("swapimage-" + std::to_string(i), imageDesc);

        images_.push_back(handle);
    }
}

void hammock::engine::Framebuffer::createCommandBuffers() {
    for (int i = 0; i < framesInFlight_; i++) {
        auto cmd = std::make_unique<core::CommandBuffer>(device_, core::CommandQueueFamily::Graphics);
        commandBuffers_.push_back(std::move(cmd));
    }
}

void hammock::engine::Framebuffer::createSemaphores() {
    for (int i = 0; i < framesInFlight_; i++) {
        auto semaphore = std::make_unique<core::Semaphore>(device_);
        framebufferReadySemaphores_.push_back(std::move(semaphore));
    }
}

hammock::core::CommandBuffer &hammock::engine::Framebuffer::getFrontBufferCommandBuffer() const {
    return *commandBuffers_[currentFrame_];
}
