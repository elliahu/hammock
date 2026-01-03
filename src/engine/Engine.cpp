module;

#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.engine.engine;
hammock::engine::Engine::Engine(LaunchMode mode) {
    // Create graphics context
    context_ = std::make_unique<renderer::GraphicsContext>(renderer::GraphicsContextDesc{
        .mode = renderer::GraphicsContextMode::Windowed,
    });

    // Create editor
    editor_ = std::make_unique<Editor>(mode, *context_);
}

void hammock::engine::Engine::launch() const {
    editor_->launch();
}
