module;

#include <memory>
#include <array>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.engine.editor;

import hammock.engine.window;
import hammock.engine.framebuffer;
import hammock.renderer.renderer;
import hammock.renderer.graphics_context;
import hammock.engine.ui;
import hammock.core.swapchain_manager;
import hammock.core.command_buffer;
import hammock.core.swapchain;
import hammock.core.device;
import hammock.core.semaphore;
import hammock.renderer.math;

namespace hammock::engine {
    /// @enum LaunchMode
    /// @brief Describes how the engine is launched
    export enum class LaunchMode {
        Editor,
        Runtime,
    };

    /// @class Editor
    /// @brief Represents engines editor
    export class Editor final {
    public:
        Editor(
            const LaunchMode mode,
            renderer::GraphicsContext &ctx);

        ~Editor();

        /// @brief Launch the editor window, this will also start the rendering
        /// This function is the entrypoint into the engine rendering
        void launch();

    private:
        void loop();

        void present() const;

        void recreateFramebuffer();

        LaunchMode mode_;
        renderer::GraphicsContext &context_;
        std::unique_ptr<renderer::Renderer> renderer_;
        std::unique_ptr<Window> window_;
        std::unique_ptr<Framebuffer> framebuffer_;
        std::vector<std::unique_ptr<core::Semaphore> > frameFinishedSemaphores_;
        std::vector<std::unique_ptr<core::CommandBuffer> > presentCommandBuffers_;
        math::Vec2 extent_;
    };
}
