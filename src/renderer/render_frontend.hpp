#pragma once
#include "render_proxy.hpp"
#include "render_types.hpp"
#include "surface_provider_iface.hpp"

namespace hammock::renderer {

    class RenderFrontend {
       public:
        explicit RenderFrontend(RenderProxy& proxy, core::SurfaceProviderIface& surfaceProvider);

        /// @brief Start the execution
        void start();

       private:
        // Handles os inputs here
        void handleInput() {};
        // Updates the state
        void update() {}
        // Builds the render snapshot
        RenderSnapshot buildRenderSnapshot();

        RenderProxy& proxy_;
        core::SurfaceProviderIface& surfaceProvider_;
        float frameTime_{0.f};
        float renderTime_{0.f};
        float cmdTime_{0.f};
    };
}  // namespace hammock::renderer
