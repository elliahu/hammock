module;

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.engine.editor:framebuffer;

import hammock.core;
import hammock.renderer;


namespace hammock::engine {
    /// @class Framebuffer
    /// @brief This class represents offscreen framebuffer used by the engine editor
    export class Framebuffer final {
    public:
        Framebuffer(core::Device& device, std::uint32_t framesInFlight, math::Vec2 resolution, vk::Format format) : device(device), framesInFlight(
            framesInFlight) {
            createImages(resolution, format);
            createCommandBuffers();
            createSemaphores();
        }

        // Framebuffer lifetime is expected to be smaller than resource managers lifetime.
        // It is also expected that framebuffer will be recreated many times .
        // This means we cannot rely on resource manager to delete the resource in its destructor.
        ~Framebuffer() {
            auto &rm = core::ResourceManager::getInstance();
            for (auto &i: images) {
                rm.releaseResource(i.getUid());
            }
            commandBuffers.clear();
            framebufferReadySemaphores.clear();
        }

        /// @brief Swap the current front buffer
        void swapImages() {
            currentFrame = (currentFrame + 1) % framesInFlight;

            auto image = core::ResourceManager::getInstance().getResource<core::Image>(getFrontbufferImage());
            auto &commandBuffer = getFrontBufferCommandBuffer();
            commandBuffer.signalSemaphore(*framebufferReadySemaphores[currentFrame]);

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

        /// @brief Get current front buffer image
        [[nodiscard]] core::ResourceHandle getFrontbufferImage() const {
            return getImage(currentFrame);
        }


        /// @brief Get image handle
        [[nodiscard]] core::ResourceHandle getImage(std::uint32_t index) const {
            if (index >= images.size()) {
                throw std::runtime_error("Invalid framebuffer image index");
            }
            return images[index];
        }

        /// @brief Get raw image pointer
        [[nodiscard]] core::Image *getImagePtr(std::uint32_t index) const {
            if (index >= images.size()) {
                throw std::runtime_error("Invalid framebuffer image index");
            }
            return core::ResourceManager::getInstance().getResource<core::Image>(images[index]);
        }

        [[nodiscard]] core::Semaphore &getFramebufferReadySemaphore() {
            return *framebufferReadySemaphores[currentFrame];
        }

    private:
        void createImages(math::Vec2 resolution, vk::Format format) {
            auto &rm = core::ResourceManager::getInstance();
            for (int i = 0; i < framesInFlight; i++) {
                auto handle = rm.createResource<core::Image>("swapimage-" + std::to_string(i), core::ImageDesc{
                                                                 .width = static_cast<std::uint32_t>(resolution.X),
                                                                 .height = static_cast<std::uint32_t>(resolution.Y),
                                                                 .channels = 4,
                                                                 .depth = 1,
                                                                 .layers = 1,
                                                                 .mips = 1,
                                                                 .format = format,
                                                                 .usage = vk::ImageUsageFlagBits::eColorAttachment |
                                                                          vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
                                                                 .imageType = vk::ImageType::e2D,
                                                                 .imageViewType = vk::ImageViewType::e2D,
                                                             });

                images.push_back(handle);
            }
        }

        void createCommandBuffers() {
            for (int i = 0; i < framesInFlight; i++) {
                auto cmd = std::make_unique<core::CommandBuffer>(device, core::CommandQueueFamily::Graphics);
                commandBuffers.push_back(std::move(cmd));
            }
        }

        void createSemaphores() {
            for (int i = 0; i < framesInFlight; i++) {
                auto semaphore = std::make_unique<core::Semaphore>(device);
                framebufferReadySemaphores.push_back(std::move(semaphore));
            }
        }

        core::CommandBuffer &getFrontBufferCommandBuffer() {
            return *commandBuffers[currentFrame];
        }

        core::Device& device;
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers;
        std::vector<std::unique_ptr<core::Semaphore>> framebufferReadySemaphores;
        std::vector<core::ResourceHandle> images;
        std::uint32_t framesInFlight = 0;
        std::uint32_t currentFrame = 0;
    };
}
