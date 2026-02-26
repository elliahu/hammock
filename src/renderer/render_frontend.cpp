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

    Ui::initialize(surfaceProvider_.getExtent().width, surfaceProvider_.getExtent().height);
    Ui::instance().setUiCallback([] {
        // Declare UI using Clay macros — runs on logic thread, no GPU state
        CLAY(CLAY_ID("Root"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(16),
                        .childGap = 8,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                .backgroundColor = {20, 20, 20, 200},
            }) {
            CLAY_TEXT(CLAY_STRING("Loading..."),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 255, 255, 255},
                    .fontId = 0,
                    .fontSize = 24,
                }));
        }
    });

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

        UiSnapshot snap = Ui::instance().lay();
        proxy_.getUserInterfaceQueue().push(snap);
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