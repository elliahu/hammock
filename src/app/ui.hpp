#pragma once
#include <compare>
#include <cstdint>

#include "VulkanSurfer/imgui_impl_vulkansurfer.h"
#include "core/vulkan_context.hpp"
#include "core/command_buffer.hpp"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"
#include "vulkan/vulkan.hpp"

namespace hammock::app {

    /// @class Ui
    /// @brief This class is responsible for laying out and rendering the ui
    class Ui {
       public:
        Ui(Surfer::Window* window, core::VulkanContext& ctx);

        ~Ui();

        /// @brief Renders the user interface on top of provided image
        void renderFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore& wait,
            core::Semaphore& signal);

       private:
        static void draw();

        core::VulkanContext& ctx_;
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;
    };
}  // namespace hammock::engine
