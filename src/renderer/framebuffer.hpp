#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "core/device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "utils/math.hpp"


namespace hammock::renderer {

    /// @class Framebuffer
    /// @brief This class represents offscreen framebuffer used by the engine editor
    class Framebuffer final {
    public:
        Framebuffer(core::Device& device, std::uint32_t framesInFlight, math::Vec2 resolution, core::ImageFormat format);

        // Framebuffer lifetime is expected to be smaller than resource managers lifetime.
        // It is also expected that framebuffer will be recreated many times .
        // This means we cannot rely on resource manager to delete the resource in its destructor.
        ~Framebuffer();

        /// @brief Get the extent of the framebuffer
        [[nodiscard]] vk::Extent2D getExtent() const;

        /// @brief Swap the current front buffer
        void swapImages();

        /// @brief Get current front buffer image
        [[nodiscard]] core::ResourceRef<core::Image> getFrontbufferImage();

    private:
        void createImages(math::Vec2 resolution, core::ImageFormat format);

        core::Device& device_;
        std::vector<core::Handle<core::Image>> handles_;
        core::ResourceManager<core::Image> images_{};
        std::uint32_t framesInFlight_ = 0;
        std::uint32_t currentFrame_ = 0;
        math::Vec2 resolution_;
    };
}
