#include "render_frontend.hpp"

#include "render_proxy.hpp"
#include "render_types.hpp"
#include "utilities.hpp"
#include "ui/ui.hpp"

hammock::renderer::RenderFrontend::RenderFrontend(
    RenderProxy& proxy, core::SurfaceProviderIface& surfaceProvider)
    : proxy_(proxy), surfaceProvider_(surfaceProvider) {}

void hammock::renderer::RenderFrontend::start() {
    core::Logger::debug("%s", "Logic thread created");

    // Send frontend ready
    proxy_.getFtbCommandQueue().push(RenderCommand{.type = RenderCommandType::Ready});

    // Enter the logic loop
    while (!surfaceProvider_.shouldClose()) {
        // Poll os events
        surfaceProvider_.pollEvents();

        // Handle resize
        if (surfaceProvider_.wasResized()) {
            // Get current extent
            auto extent = surfaceProvider_.getExtent();
            // Send resize command
            RenderCommand cmd{
                .type = RenderCommandType::Resize, .width = extent.width, .height = extent.height};
            proxy_.getFtbCommandQueue().push(std::move(cmd));
            // Inform ui about the resize
            ui::Ui::instance().setDisplaySize(extent.width, extent.height);
            // Resize handled
            surfaceProvider_.resetResized();
        }

        handleInput();
        update();

        // Send the render snap
        RenderSnapshot snap = buildRenderSnapshot();
        ui::Ui::instance().lay(snap.uiDraws);
        proxy_.getRenderQueue().push(snap);
    }

    // Notify close
    core::Logger::debug("%s", "Window closed, sending stop command to backend");
    proxy_.getFtbCommandQueue().push(RenderCommand{.type = RenderCommandType::Stop});
}

hammock::renderer::RenderSnapshot hammock::renderer::RenderFrontend::buildRenderSnapshot() {
    auto packet = RenderSnapshot{};
    x += .001f;
    packet.x = static_cast<uint32_t>(x) % 255;
    return packet;
}
