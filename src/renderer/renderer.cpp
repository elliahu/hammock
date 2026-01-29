#include <memory>
#include <stdexcept>

#include "renderer.hpp"
#include "hammock_core.hpp"

hammock::renderer::Renderer::Renderer(std::unique_ptr<BaseRenderingStrategy> &&strategy): strategy_(std::move(strategy)) {
}

void hammock::renderer::Renderer::drawFrame(core::ResourceHandle target, std::uint32_t frameIndex,
    core::Semaphore &signal) const {
    if (!target.isValid() || target.getType() != core::ResourceType::Image) {
        throw std::runtime_error("Invalid rendering target");
    }

    strategy_->draw(target, frameIndex, signal);
}
