#pragma once
#include <memory>

#include "runner.hpp"
#include "hammock_renderer.hpp"


namespace hammock::engine {
    class Application final {
    public:
        explicit Application(RunnerMode mode);

        void launch() const;

    private:
        std::unique_ptr<renderer::GraphicsContext> context_;
        std::unique_ptr<Runner> runner_;
    };
}
