module;

#include <stdexcept>
#include <vulkan/vulkan.h>

export module hammock.core.semaphore;

import hammock.core.device;

namespace hammock::core {
    export class Semaphore final {
    public:
        explicit Semaphore(Device &device) : device(device) {
            VkSemaphoreCreateInfo semaphoreCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };
            if (auto result = vkCreateSemaphore(device.device(), &semaphoreCreateInfo, nullptr, &semaphore);
                result != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphore");
            }
        }

        ~Semaphore() {
            vkDestroySemaphore(device.device(), semaphore, nullptr);
        }

        [[nodiscard]] VkSemaphore getVulkanSemaphore() const {
            return semaphore;
        }

    private:
        Device &device;
        VkSemaphore semaphore;
    };
}
