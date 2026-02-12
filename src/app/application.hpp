#pragma once
#include <memory>

#include "runner.hpp"
#include "core/vulkan_context.hpp"


namespace hammock::app {
    class Application final {
    public:
        explicit Application(RunnerMode mode);

        void launch() const;

    private:
        std::unique_ptr<core::VulkanContext> context_;
        std::unique_ptr<Runner> runner_;
    };
}
