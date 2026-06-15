#include "runner.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vulkan/vulkan.hpp>

#include "VulkanSurfer/VulkanSurfer.h"
#include "presenter.hpp"
#include "rendering_server.hpp"
#include "dispatcher.hpp"
#include "ui/ui.hpp"

hammock::app::Runner::Runner() {
    // Create window
    window_ = std::make_unique<Window>("Hammock", 1920u, 1080u);

    // Init ui
    ui::Ui::initialize(window_->getExtent().width, window_->getExtent().height);

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
    renderingServerProxy_ = std::make_unique<app::RenderingServerProxy>();
    dispatcher_ = std::make_unique<app::Dispatcher>(*renderingServerProxy_, *window_, ringBuffer);

    // Initialize renderer
    auto renderer = std::make_unique<renderer::Renderer>(
        [&](core::Instance& instance) {
            window_->createVulkanSurface(instance);
            return window_->getSurface();
        },
        [&](core::Instance& instance, vk::SurfaceKHR surface) { window_->destroyVulkanSurface(instance); });

    // Initialize presenter
    auto presenter =
        std::make_unique<app::Presenter>(renderer->getDevice(), *window_, app::PresenterDesc{});

    // Set up resolution change callback that will resize ui
    presenter->onResolutionChanged(
        [this](uint32_t width, uint32_t height) { ui::Ui::instance().setDisplaySize(width, height); });

    // Create render backend by moving the renderer
    renderingServer_ = std::make_unique<app::RenderingServer>(
        *renderingServerProxy_, *window_, std::move(renderer), std::move(presenter));
}

hammock::app::Runner::~Runner() {}

void hammock::app::Runner::run() {
    // Backend must start before frontend as the frontend runs on this (main) thread
    renderingServer_->start();
    // Start the frontend
    dispatcher_->start();

    // Join the threads
    renderingServer_->join();
}
