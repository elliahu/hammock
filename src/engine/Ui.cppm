module;

#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui_impl_vulkansurfer.h"
#include "imgui/imgui.h"
#include <compare>
#include "vulkan/vulkan.hpp"


export module hammock.engine.ui;

import hammock.core.command_buffer;
import hammock.core.descriptor;
import hammock.renderer.graphics_context;

namespace hammock::engine {

    /// @class Ui
    /// @brief This class is responsible for laying out and rendering the ui
    export class Ui {
    public:
        Ui(Surfer::Window * window, renderer::GraphicsContext * ctx);

        ~Ui();

        /// @brief Renders the user interface on top of provided image
        static void renderFrame(core::CommandBuffer& commandBuffer, vk::ImageView swapchainImageView, std::uint32_t width, std::uint32_t height);

    private:
        static void draw();
    };
}
