#include "runner.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vulkan/vulkan.hpp>

#include "VulkanSurfer/VulkanSurfer.h"
#include "render_backend.hpp"
#include "render_frontend.hpp"
#include "render_proxy.hpp"
#include "render_types.hpp"
#include "ui/ui.hpp"
#include "ui/ui.hpp"

hammock::app::Runner::Runner() {
    // Create window
    window_ = std::make_unique<Window>("Hammock", 1920u, 1080u);

    // Init ui
    ui::Ui::initialize(window_->getExtent().width, window_->getExtent().height);

    ui::Ui::instance().setUiCallback([] {
        CLAY(CLAY_ID("Root"),
            {
                .layout =
                    {
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

    // Create render proxy and frontend
    renderProxy_ = std::make_unique<renderer::RenderProxy>();
    renderFrontend_ = std::make_unique<renderer::RenderFrontend>(*renderProxy_, *window_);

    // Initialize renderer
    auto renderer = std::make_unique<renderer::Renderer>([&](core::Instance& instance) {
        window_->createVulkanSurface(instance);
        return window_->getSurface();
    }, [&](core::Instance& instance, vk::SurfaceKHR surface) {
        window_->destroyVulkanSurface(instance);
    });

    // Initialize presenter
    auto presenter = std::make_unique<renderer::Presenter>(renderer->getDevice(), *window_);

    // Create render backend by moving the renderer
    renderBackend_ = std::make_unique<renderer::RenderBackend>(*renderProxy_, *window_, std::move(renderer), std::move(presenter));
}

hammock::app::Runner::~Runner() {
}

void hammock::app::Runner::launch() {
    // Backend must start before frontend as the frontend runs on this (main) thread
    renderBackend_->start();
    // Start the frontend
    renderFrontend_->start();

    // Join the threads
    renderBackend_->join();
}
