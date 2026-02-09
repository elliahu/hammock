#pragma once
#include <memory>
#include <stdexcept>

#include "graphics_context.hpp"
#include "hammock_core.hpp"

namespace hammock::renderer {
    class Renderer {
       public:
        explicit Renderer(GraphicsContext& context);

        void drawFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore& signal) const;

       private:
        GraphicsContext& ctx_;
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;
    };
};  // namespace hammock::renderer
