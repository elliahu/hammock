#pragma once
#include <compare>
#include <vulkan/vulkan.hpp>
#include "instance.hpp"

namespace hammock::core {

    class SurfaceProviderIface {
       public:
        virtual ~SurfaceProviderIface() = default;
        virtual vk::Extent2D getExtent() const = 0;
        virtual vk::SurfaceKHR getSurface() const = 0;
        virtual bool wasResized() const = 0;
        virtual bool shouldClose() const = 0;
        virtual void pollEvents() const = 0;
        virtual void resetResized() = 0;
        virtual void createVulkanSurface(Instance& instance) = 0;
        virtual void destroyVulkanSurface(Instance& instance) = 0;
    };
}  // namespace hammock::core