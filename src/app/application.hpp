#pragma once
#include <memory>

#include "application_types.hpp"
#include "core/vulkan_context.hpp"
#include "runner.hpp"

namespace hammock::app {
    class Application final {
       public:
        explicit Application(ExecutionMode mode);

        void launch() const;
        core::VulkanContext& getVulkanContext() { return *context_; }

       private:
        std::unique_ptr<core::VulkanContext> context_;
        std::unique_ptr<Runner> runner_;
    };
}  // namespace hammock::app
