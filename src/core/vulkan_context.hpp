#pragma once
#include <compare>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "surface_provider_iface.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "resource_manager.hpp"

namespace hammock::core {

    struct VulkanContext {
        std::unique_ptr<core::Instance> instance{nullptr};
        std::unique_ptr<core::Device> device{nullptr};
        std::unique_ptr<core::ResourceManager> resourceManager{nullptr};
        std::unique_ptr<core::DescriptorPool> descriptorPool{nullptr};

        /// @brief Create the vulkan instance
        /// @note To fully initialize the context, call initialize
        VulkanContext();

        /// @brief This initializes the full vulkan context
        /// @note If this is not called, only the vulkan instance is created
        void initialize(core::SurfaceProviderIface& surfaceProvider);
    };
}  // namespace hammock::core
