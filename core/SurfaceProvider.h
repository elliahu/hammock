#pragma once
#include <vulkan/vulkan.h>

namespace Hammock {

    class SurfaceProvider {
       public:
        virtual ~SurfaceProvider() = default;
        virtual VkExtent2D getExtent() const = 0;
        virtual VkSurfaceKHR getSurface() const = 0;
        virtual bool wasResized() const = 0;
        virtual void resetResized() = 0;
    };
}  // namespace Hammock