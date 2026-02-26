#include "render_frontend.hpp"

#include <stdexcept>

#include "render_proxy.hpp"
#include "render_types.hpp"
#include "utilities.hpp"

hammock::renderer::RenderFrontend::RenderFrontend(
    RenderProxy& proxy, core::SurfaceProviderIface& surfaceProvider)
    : proxy_(proxy), surfaceProvider_(surfaceProvider) {}

void hammock::renderer::RenderFrontend::start() {
    core::Logger::debug("Logic thread %d", std::this_thread::get_id());

    // Send frontend ready
    proxy_.getFtbCommandQueue().push(RenderCommand{.type = RenderCommandType::Ready});

    // Enter the logic loop
    while (!surfaceProvider_.shouldClose()) {
        // Poll os events
        surfaceProvider_.pollEvents();

        // Handle resize
        if (surfaceProvider_.wasResized()) {
            auto extent = surfaceProvider_.getExtent();
            RenderCommand cmd{
                .type = RenderCommandType::Resize, .width = extent.width, .height = extent.height};
            proxy_.getFtbCommandQueue().push(std::move(cmd));
            surfaceProvider_.resetResized();
        }

        handleInput();
        update();

        // Send the render snap
        proxy_.getRenderQueue().push(buildRenderSnapshot());
    }

    // Notify close
    proxy_.getFtbCommandQueue().push(RenderCommand{.type = RenderCommandType::Stop});
}

hammock::renderer::RenderSnapshot hammock::renderer::RenderFrontend::buildRenderSnapshot() {
    auto packet = RenderSnapshot{};
    x += .001f;
    packet.x = static_cast<uint32_t>(x) % 255;
    return packet;
}