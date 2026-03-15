#pragma once

#include <memory>
#include <thread>

#include "presenter.hpp"
#include "render_proxy.hpp"
#include "renderer.hpp"
#include "surface_provider_iface.hpp"

namespace hammock::renderer {
    class RenderBackend {
       public:
        explicit RenderBackend(RenderProxy& proxy, core::SurfaceProviderIface& surfaceProvider, std::unique_ptr<RendererIface>&& renderer, std::unique_ptr<PresenterIface> &&presenter);

        void start();

        void stop();

        void join();

        bool isRunning() const;

       private:
        RenderProxy& proxy_;
        std::atomic<bool> running_{false};
        std::thread thread_;
        core::SurfaceProviderIface& surfaceProvider_;
        std::unique_ptr<RendererIface> renderer_{nullptr};
        std::unique_ptr<PresenterIface> presenter_{nullptr};
        bool readySent_{false};
    };
}  // namespace hammock::renderer
