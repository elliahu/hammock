module;

#include "imgui/backends/imgui_impl_vulkan.h"
#include "VulkanSurfer/imgui_impl_vulkansurfer.h"
#include "imgui/imgui.h"
#include <cstdint>
#include <compare>
#include "vulkan/vulkan.hpp"


export module hammock_engine.ui;

import hammock_core;
import hammock_renderer.graphics_context;

namespace hammock::engine {

    /// @class Ui
    /// @brief This class is responsible for laying out and rendering the ui
    export class Ui {
    public:
        Ui(Surfer::Window * window, renderer::GraphicsContext * ctx, std::uint32_t framesInFlight);

        ~Ui();

        /// @brief Renders the user interface on top of provided image
        void renderFrame(core::ResourceHandle target, std::uint32_t frameIndex,  core::Semaphore &wait, core::Semaphore &signal);

    private:
        static void draw();


        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;

    };
}
