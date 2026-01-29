#pragma once
#include <memory>
#include <stdexcept>

#include "base_rendering_strategy.hpp"
#include "hammock_core.hpp"

namespace hammock::renderer {
    class Renderer {
       public:
        explicit Renderer(std::unique_ptr<BaseRenderingStrategy>&& strategy = {});

        void drawFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore& signal) const;

       private:
        std::unique_ptr<BaseRenderingStrategy> strategy_{};
        ;
    };
};  // namespace hammock::renderer
