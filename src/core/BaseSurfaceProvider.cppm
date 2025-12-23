module;


export module hammock.core.base_surface_provider;

import vulkan_hpp;

namespace hammock::core {

    export class BaseSurfaceProvider {
       public:
        virtual ~BaseSurfaceProvider() = default;
        virtual vk::Extent2D getExtent() const = 0;
        virtual vk::SurfaceKHR getSurface() const = 0;
        virtual bool wasResized() const = 0;
        virtual void resetResized() = 0;
    };
}  // namespace hammock::core