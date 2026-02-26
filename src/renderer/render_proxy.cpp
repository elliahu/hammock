#include "render_proxy.hpp"

hammock::renderer::RenderQueue& hammock::renderer::RenderProxy::getRenderQueue() { return renderQueue_; }
hammock::renderer::CommandQueue& hammock::renderer::RenderProxy::getFtbCommandQueue() { return ftbCommandQueue_; }
hammock::renderer::CommandQueue& hammock::renderer::RenderProxy::getBtfCommandQueue() { return btfCommandQueue_; }
