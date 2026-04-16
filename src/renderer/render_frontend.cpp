#include "render_frontend.hpp"

#include "rendering_server.hpp"
#include "render_types.hpp"
#include "utilities.hpp"
#include "ui/ui.hpp"

hammock::renderer::RenderFrontend::RenderFrontend(
    RenderingServerProxy& proxy, core::SurfaceProviderIface& surfaceProvider)
    : proxy_(proxy), surfaceProvider_(surfaceProvider) {}

void hammock::renderer::RenderFrontend::start() {
    // Singnal render thread that logic thread is ready
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
        }

        // Recieve messages from backend
        RenderCommand message;
        proxy_.getBtfCommandQueue().pop(message);
        if(message.type == RenderCommandType::Frametime){
            frameTime_ = message.frametime;
            renderTime_ = message.rendertime;
            cmdTime_ = message.cmdtime;
        }

        handleInput();
        update();

        // Send the render snap
        RenderSnapshot snap = buildRenderSnapshot();
        ui::Ui::instance().beginLayout();
        std::string frameTimeStr = std::format("Frame total: {:.2f} ms, Render {:.4f}, Cmd {:.4f}", frameTime_, renderTime_, cmdTime_);

        // Fullscreen background
        CLAY(CLAY_ID("Screen"),
        {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(),
                    .height = CLAY_SIZING_GROW(),
                }
            },
            .backgroundColor = {40, 40, 40, 125}, // background color
        })
        {
            // Your existing hoverable rectangle
            CLAY(CLAY_ID("Root"),
            {
                .layout = {
                    .padding = CLAY_PADDING_ALL(16),
                    .childGap = 8,
                },
                .backgroundColor =
                    Clay_Hovered()
                        ? Clay_Color{255,165,0,255}
                        : Clay_Color{255,255,255,255},
            })
            {
                // Construct a Clay_String manually from the std::string
                Clay_String clayFrameTimeStr = {
                    .length = static_cast<int32_t>(frameTimeStr.length()),
                    .chars = frameTimeStr.c_str()
                };

                CLAY_TEXT(clayFrameTimeStr,
                CLAY_TEXT_CONFIG({
                    .textColor = {0,0,0,255},
                    .fontId = 0,
                    .fontSize = 12,
                }));
            }
        }
        ui::Ui::instance().endLayout(snap.uiDraws);
        proxy_.getRenderQueue().push(snap);
    }

    // Notify close
    core::Logger::debug("%s", "Window closed, sending stop command to backend");
    proxy_.getFtbCommandQueue().push(RenderCommand{.type = RenderCommandType::Stop});
}

hammock::renderer::RenderSnapshot hammock::renderer::RenderFrontend::buildRenderSnapshot() {
    auto packet = RenderSnapshot{};
    return packet;
}
