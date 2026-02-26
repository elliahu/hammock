#pragma once

#include <memory>
#include <thread>

#include "presentation_engine.hpp"
#include "render_proxy.hpp"
#include "render_types.hpp"
#include "renderer.hpp"
#include "surface_provider_iface.hpp"
#include "vulkan_context.hpp"

namespace hammock::renderer {
    class RenderBackend {
       public:
        explicit RenderBackend(
            RenderProxy& proxy, core::VulkanContext& ctx, core::SurfaceProviderIface& surfaceProvider);

        void start();

        void stop();

        void join();

        bool isRunning() const;

       private:
        RenderProxy& proxy_;
        std::atomic<bool> running_{false};
        core::VulkanContext& ctx_;
        std::thread thread_;
        core::SurfaceProviderIface& surfaceProvider_;
        std::unique_ptr<PresentationEngine> presentationEngine_{nullptr};
        std::unique_ptr<RendererIface> renderer_;
        bool readySent_{false};
    };
}  // namespace hammock::renderer