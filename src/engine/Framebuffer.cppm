module;

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.engine.framebuffer;

import hammock.core;
import hammock.renderer;


namespace hammock::engine {

    /// @class Framebuffer
    /// @brief This class represents offscreen framebuffer used by the engine editor
    export class Framebuffer final {
    public:
        Framebuffer(core::Device& device, std::uint32_t framesInFlight, math::Vec2 resolution, vk::Format format);

        // Framebuffer lifetime is expected to be smaller than resource managers lifetime.
        // It is also expected that framebuffer will be recreated many times .
        // This means we cannot rely on resource manager to delete the resource in its destructor.
        ~Framebuffer();

        /// @brief Get the extent of the framebuffer
        vk::Extent2D getExtent() const;

        /// @brief Swap the current front buffer
        void swapImages();

        /// @brief Get current front buffer image
        [[nodiscard]] core::ResourceHandle getFrontbufferImage() const;


        /// @brief Get image handle
        [[nodiscard]] core::ResourceHandle getImage(std::uint32_t index) const;

        /// @brief Get raw image pointer
        [[nodiscard]] core::Image *getImagePtr(std::uint32_t index) const;

        [[nodiscard]] core::Semaphore &getFramebufferReadySemaphore() const;

    private:
        void createImages(math::Vec2 resolution, vk::Format format);

        void createCommandBuffers();

        void createSemaphores();

        core::CommandBuffer &getFrontBufferCommandBuffer() const;

        core::Device& device_;
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;
        std::vector<std::unique_ptr<core::Semaphore>> framebufferReadySemaphores_;
        std::vector<core::ResourceHandle> images_;
        std::uint32_t framesInFlight_ = 0;
        std::uint32_t currentFrame_ = 0;
        math::Vec2 resolution_;
    };
}
