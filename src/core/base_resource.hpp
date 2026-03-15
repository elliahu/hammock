#pragma once
#include <vulkan/vulkan.hpp>

#include "device.hpp"

namespace hammock::core {

    // Base resource class
    class BaseResource {
       protected:
        Device& device;
        vk::DeviceSize size = 0;
        bool resident;  // Whether the resource is currently in GPU memory

        BaseResource(Device& device) : device(device), resident(false) {}

        // Allow moving but not copying
        BaseResource(const BaseResource&) = delete;
        BaseResource& operator=(const BaseResource&) = delete;

        BaseResource(BaseResource&&) = default;
        BaseResource& operator=(BaseResource&&) = delete;

       public:
        virtual ~BaseResource() {}

        virtual void create() = 0;

        virtual void release() = 0;

        inline bool isResident() const { return resident; }
        [[nodiscard]] vk::DeviceSize getSize() const { return size; }  // for now
    };
}  // namespace hammock::core
