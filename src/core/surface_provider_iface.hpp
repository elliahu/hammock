#pragma once
#include <compare>
#include <vulkan/vulkan.hpp>

namespace hammock::core {

    class SurfaceProviderIface {
       public:
        virtual ~SurfaceProviderIface() = default;
        virtual vk::Extent2D getExtent() const = 0;
        virtual vk::SurfaceKHR getSurface() const = 0;
        virtual bool wasResized() const = 0;
        virtual void resetResized() = 0;
    };
}  // namespace hammock::core