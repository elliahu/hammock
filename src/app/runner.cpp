#include "runner.hpp"

#include <cstdint>
#include <memory>
#include <thread>
#include <vulkan/vulkan.hpp>

#include "VulkanSurfer/VulkanSurfer.h"
#include "application_types.hpp"
#include "core/vulkan_context.hpp"
#include "render_backend.hpp"
#include "render_frontend.hpp"
#include "render_proxy.hpp"
#include "render_types.hpp"
#include "thread_pool.hpp"
#include "ui.hpp"

hammock::app::Runner::Runner(ExecutionMode mode, core::VulkanContext& context) : vulkanContext_(context) {
    // Create window
    window_ = std::make_unique<Window>("Hammock", *context.instance, 1920u, 1080u);

    // Attach surface
    vulkanContext_.initialize(*window_);

    // Init ui
    ui::Ui::initialize(window_->getExtent().width, window_->getExtent().height);

    ui::Ui::instance().setUiCallback([] {
        // Declare UI using Clay macros — runs on logic thread, no GPU state
        CLAY(CLAY_ID("Root"),
            {
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_FIT({}), .height = CLAY_SIZING_FIXED(50)},
                        .padding = CLAY_PADDING_ALL(16),
                        .childGap = 8,
                    },
                .backgroundColor =
                    Clay_Hovered() ? Clay_Color{255, 165, 0, 255} : Clay_Color{255, 255, 255, 255},
                .clip = {.vertical = true, .childOffset = Clay_GetScrollOffset()},
            }, ) {
            CLAY_TEXT(CLAY_STRING("Small black text"),
                CLAY_TEXT_CONFIG({
                    .textColor = {0, 0, 0, 255},
                    .fontId = 0,
                    .fontSize = 12,
                }));
            CLAY_TEXT(CLAY_STRING("Large red text"),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 0, 0, 255},
                    .fontId = 0,
                    .fontSize = 24,
                }));
            CLAY_TEXT(CLAY_STRING("Large red text"),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 0, 0, 255},
                    .fontId = 0,
                    .fontSize = 24,
                }));
            CLAY_TEXT(CLAY_STRING("Large red text"),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 0, 0, 255},
                    .fontId = 0,
                    .fontSize = 24,
                }));
            CLAY_TEXT(CLAY_STRING("Large red text"),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 0, 0, 255},
                    .fontId = 0,
                    .fontSize = 24,
                }));
        }
    });

    window_->getWindowPtr()->registerMouseMotionCallback([this](uint32_t x, uint32_t y) {
        ui::Ui::instance().setPointerPosition({static_cast<float>(x), static_cast<float>(y)});
    });

    window_->getWindowPtr()->registerKeyPressCallback([this](Surfer::KeyCode key) {
        if (key == Surfer::KeyCode::MouseLeft) {
            ui::Ui::instance().setPointerDown(true);
        } else if (key == Surfer::KeyCode::MouseWheelUp) {
            ui::Ui::instance().setScrollDelta({0, 1});
        } else if (key == Surfer::KeyCode::MouseWheelDown) {
            ui::Ui::instance().setScrollDelta({0, -1});
        }
    });

    window_->getWindowPtr()->registerKeyReleaseCallback([this](Surfer::KeyCode key) {
        if (key == Surfer::KeyCode::MouseLeft) {
            ui::Ui::instance().setPointerDown(false);
        } else if (key == Surfer::KeyCode::MouseWheelUp) {
            ui::Ui::instance().setScrollDelta({0, 0});
        } else if (key == Surfer::KeyCode::MouseWheelDown) {
            ui::Ui::instance().setScrollDelta({0, 0});
        }
    });

    window_->getWindowPtr()->registerMouseEnterExitCallback([this](bool entered) {
        if (entered) {
            uint32_t x, y;
            window_->getWindowPtr()->getCursorPosition(x, y);
            ui::Ui::instance().setPointerPosition({static_cast<float>(x), static_cast<float>(y)});
        } else {
            ui::Ui::instance().setPointerPosition({-1, -1});
        }
    });

    /// Set the threads
    renderProxy_ = std::make_unique<renderer::RenderProxy>();
    renderFrontend_ = std::make_unique<renderer::RenderFrontend>(*renderProxy_, *window_);
    renderBackend_ = std::make_unique<renderer::RenderBackend>(*renderProxy_, vulkanContext_, *window_);
}

hammock::app::Runner::~Runner() {
    // Wait for GPU to finish before destroying resources
    vulkanContext_.device->waitIdle();
}

void hammock::app::Runner::launch() {
    core::Logger::debug("Main thread %d", std::this_thread::get_id());
    // Backend must start before frontend as the frontend runs on this (main) thread
    renderBackend_->start();
    // Start the frontend
    renderFrontend_->start();

    // Join the threads
    renderBackend_->join();
}
