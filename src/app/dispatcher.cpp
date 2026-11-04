#include "dispatcher.hpp"

#include <cstring>
#include <vector>

#include "frame_arena.hpp"
#include "render_snapshot.hpp"
#include "rendering_server.hpp"
#include "ui/ui.hpp"
#include "utilities.hpp"
#include "vertex.hpp"

hammock::app::Dispatcher::Dispatcher(
    RenderingServerProxy& proxy, core::SurfaceProviderIface& surfaceProvider, RenderRingBuffer& ringBuffer)
    : renderingServerProxy_(proxy), surfaceProvider_(surfaceProvider), ringBuffer_{ ringBuffer } {}

void hammock::app::Dispatcher::start() {
    // Enter the logic loop
    while (!surfaceProvider_.shouldClose()) {
        // Wait for backend to catch up
        waitForBackend();

        // Grab this frames dedicated arena and erase previous contents
        FrameArena& arena = ringBuffer_.getFrameArena(frameNumber_);
        arena.reset();

        // Set up the header at the very front of the buffer
        auto* header = reinterpret_cast<renderer::RenderSnapshotHeader*>(arena.buffer.data());
        header->frameNumber = frameNumber_;

        // Poll os events
        surfaceProvider_.pollEvents();

        // Handle resize
        if (surfaceProvider_.wasResized()) {
            // Get current extent
            auto extent = surfaceProvider_.getExtent();
            // Send resize command
            header->view.width = extent.width;
            header->view.height = extent.height;
        }

        // Receive messages from backend
        RenderingServerInfoCommand message;
        while (renderingServerProxy_.getResponseQueue().pop(message)) {
            if (message.type == RenderingServerInfoCommandType::Frametime) {
                frameTime_ = message.frametime;
                renderTime_ = message.rendertime;
                cmdTime_ = message.cmdtime;
            }
        }

        handleInput();
        update();

        // Build the render snap
        std::vector<renderer::UiVertex> uiVertices{};
        ui::Ui::instance().beginLayout();
        std::string frameTimeStr = std::format(
            "Frame total: {:.2f} ms, Render {:.4f}, Cmd {:.4f}", frameTime_, renderTime_, cmdTime_);

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
                Clay_String clayFrameTimeStr = { .length = static_cast<int32_t>(frameTimeStr.length()),
                    .chars = frameTimeStr.c_str() };

                CLAY_TEXT(clayFrameTimeStr,
                    CLAY_TEXT_CONFIG({
                        .textColor = { 0, 0, 0, 255 },
                        .fontId = 0,
                        .fontSize = 12,
                    }));
            }
        }
        ui::Ui::instance().endLayout(uiVertices);

        {  // Place ui data in the arena
            size_t sizeBytes = uiVertices.size() * sizeof(renderer::UiVertex);
            uint8_t* dstMemory = arena.allocate(sizeBytes, alignof(renderer::UiVertex));
            std::memcpy(dstMemory, uiVertices.data(), sizeBytes);

            uint32_t offseetBytes = static_cast<uint32_t>(dstMemory - arena.buffer.data());
            header->uiDraws.vertices = { offseetBytes, static_cast<uint32_t>(uiVertices.size()) };
        }

        // Dispatch to backend
        RenderingServerDrawCommand drawCommand{ frameNumber_, header };
        renderingServerProxy_.getDrawQueue().push(drawCommand);
        // core::Logger::debug("Dispatcher: Sent frame %d", frameNumber_);

        // Update frame number
        frameNumber_++;
    }

    // Notify close
    core::Logger::debug("%s", "Window closed, sending poison pill to backend");
    // Poison pill
    renderingServerProxy_.getDrawQueue().push(
        RenderingServerDrawCommand{ .frameNumber = 0, .snapHeader = nullptr });
}
void hammock::app::Dispatcher::waitForBackend() {
    if (frameNumber_ >= core::SwapChain::MAX_FRAMES_IN_FLIGHT) {
        uint64_t targetFrame = frameNumber_ - core::SwapChain::MAX_FRAMES_IN_FLIGHT;
        while (renderingServerProxy_.getLastCompletedFrame() < targetFrame) {
            std::this_thread::yield();
        }
    }
}

void hammock::app::Dispatcher::update() {}

hammock::renderer::RenderSnapshotHeader hammock::app::Dispatcher::buildRenderSnapshot() {
    auto packet = renderer::RenderSnapshotHeader{};
    return packet;
}
