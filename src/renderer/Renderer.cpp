module;

#include <memory>
#include <stdexcept>

module hammock.renderer.renderer;

hammock::renderer::Renderer::Renderer(std::unique_ptr<IRenderingStrategy> &&strategy): strategy_(std::move(strategy)) {
}

void hammock::renderer::Renderer::drawFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &wait,
    core::Semaphore &signal) const {
    if (!target.isValid() || target.getType() != core::ResourceType::Image) {
        throw std::runtime_error("Invalid rendering target");
    }

    strategy_->draw(target, frameIndex, wait, signal);
}
