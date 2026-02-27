#pragma once
#include <cstddef>
#include <memory>
#include <array>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "core/vulkan_context.hpp"

#include "render_backend.hpp"
#include "render_frontend.hpp"
#include "render_proxy.hpp"
#include "window.hpp"
#include "ui/ui.hpp"
#include "renderer/presentation_engine.hpp"
#include "application_types.hpp"

namespace hammock::app {

    /// @class Runner
    /// @brief High-level application coordinator
    class Runner {
    public:
        Runner(ExecutionMode mode, core::VulkanContext& vulkanContext);
        ~Runner();

        /// @brief Start the editor main loop
        void launch();

    private:
        // Core Systems (in order of ownership dependency)

        /// Graphics context (reference, owned by Application)
        core::VulkanContext& vulkanContext_;

        /// Window (owned by Editor)
        std::unique_ptr<Window> window_;

        /// Render proxy
        std::unique_ptr<renderer::RenderProxy> renderProxy_{nullptr};

        /// Render frontend
        std::unique_ptr<renderer::RenderFrontend> renderFrontend_{nullptr};

        /// Render backend
        std::unique_ptr<renderer::RenderBackend> renderBackend_{nullptr};
    };
}
