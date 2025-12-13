module;
#include <vulkan/vulkan.h>


export module hammock.core.base_surface_provider;

namespace hammock::core {

    export class BaseSurfaceProvider {
       public:
        virtual ~BaseSurfaceProvider() = default;
        virtual VkExtent2D getExtent() const = 0;
        virtual VkSurfaceKHR getSurface() const = 0;
        virtual bool wasResized() const = 0;
        virtual void resetResized() = 0;
    };
}  // namespace hammock::core