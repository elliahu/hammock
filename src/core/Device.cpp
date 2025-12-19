module;
#include <stdexcept>
#include <set>
#include <vector>

#include <vulkan/vulkan.h>

module hammock.core.device;

import hammock.core.utilities;
import hammock.core.memory_allocator;

namespace hammock::core {
    // class member functions
    Device::Device(Instance &instance, VkSurfaceKHR surface) : instance{instance}, surface_{surface} {
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPools();
        createMemoryAllocator();
    }

    Device::~Device() {
        allocator::destroyAllocator(allocator_);
        vkDestroyCommandPool(device_, graphicsCommandPool, nullptr);
        vkDestroyCommandPool(device_, transferCommandPool, nullptr);
        vkDestroyCommandPool(device_, computeCommandPool, nullptr);
        vkDestroyDevice(device_, nullptr);
    }




    void Device::pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        Logger::info("Device count: %d", deviceCount);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, devices.data());

        for (const auto &device: devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        Logger::info("Physical device: %s", physicalDeviceProperties.deviceName);

        if (runningHeadless()) {
            Logger::info("Running in headless mode");
        }
    }

    void Device::createLogicalDevice() {
        auto queFamilyIndices = findQueueFamilies(
            physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            queFamilyIndices.graphicsFamily, queFamilyIndices.presentFamily, queFamilyIndices.transferFamily,
            queFamilyIndices.computeFamily
        };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE;

        // Create the physical device features structures

        VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features{};
        sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        sync2Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
        descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        // Enable non-uniform indexing
        descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
        descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
        descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;

        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        // Chain the features structures
        descriptorIndexingFeatures.pNext = &dynamicRenderingFeatures;
        dynamicRenderingFeatures.pNext = &sync2Features;

        // Populate VkPhysicalDeviceFeatures2
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &descriptorIndexingFeatures;
        deviceFeatures2.features = deviceFeatures;

        // Include deviceFeatures2 in VkDeviceCreateInfo pNext field
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr;
        // Ensure no enabled features from previous Vulkan versions are left active
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pNext = &deviceFeatures2; // Pass the device features structure
        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device_, queFamilyIndices.graphicsFamily, 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, queFamilyIndices.presentFamily, 0, &presentQueue_);
        vkGetDeviceQueue(device_, queFamilyIndices.transferFamily, 0, &transferQueue_);
        vkGetDeviceQueue(device_, queFamilyIndices.computeFamily, 0, &computeQueue_);
    }

    void Device::createCommandPools() {
        auto queFamilyIndices =
                findPhysicalQueueFamilies();

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queFamilyIndices.graphicsFamily;
        poolInfo.flags =
                VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device_, &poolInfo, nullptr, &graphicsCommandPool) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

        poolInfo.queueFamilyIndex = queFamilyIndices.transferFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device_, &poolInfo, nullptr, &transferCommandPool) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create transfer command pool!");
        }

        poolInfo.queueFamilyIndex = queFamilyIndices.computeFamily;

        if (vkCreateCommandPool(device_, &poolInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute command pool!");
        }
    }

    void Device::createMemoryAllocator() {
        allocator::VulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        allocator::AllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = allocator::AllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device_;
        allocatorCreateInfo.instance = instance.getInstance();
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        try {
            allocator::createAllocator(&allocatorCreateInfo, &allocator_);
        } catch (std::runtime_error &e) {
            throw;
        }
    }

    bool Device::isDeviceSuitable(VkPhysicalDevice device) {
        const QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = runningHeadless();
        if (extensionsSupported && !runningHeadless()) {
            const auto [capabilities, formats, presentModes] = querySwapChainSupport(device);
            swapChainAdequate = !formats.empty() && !presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate &&
               supportedFeatures.samplerAnisotropy;
    }


    bool Device::checkDeviceExtensionSupport(const VkPhysicalDevice device) const {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &extensionCount,
            availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension: availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices Device::findQueueFamilies(const VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            if (queueFamily.queueCount > 0) {
                // Graphics queue
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                    indices.graphicsFamilyHasValue = true;
                    graphicsQueueFamilyIndex_ = i;
                }

                // Present queue
                VkBool32 presentSupport = false;
                // Only ask for present queue when using surface
                if (!runningHeadless()) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
                    if (presentSupport) {
                        indices.presentFamily = i;
                        indices.presentFamilyHasValue = true;
                    }
                }


                // Compute queue (preferably separate from graphics)
                if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                    !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    indices.computeFamily = i;
                    indices.computeFamilyHasValue = true;
                    computeQueueFamilyIndex_ = i;
                }

                // Transfer queue (preferably separate from graphics and compute)
                if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                    !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                    !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    indices.transferFamily = i;
                    indices.transferFamilyHasValue = true;
                    transferQueueFamilyIndex_ = i;
                }
            }

            i++;
        }

        // Fallbacks: Use graphics queue for compute/transfer if no dedicated queues are found
        if (!indices.computeFamilyHasValue) {
            for (i = 0; i < queueFamilies.size(); i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    indices.computeFamily = i;
                    indices.computeFamilyHasValue = true;
                    computeQueueFamilyIndex_ = i;
                    break;
                }
            }
        }

        if (!indices.transferFamilyHasValue) {
            for (i = 0; i < queueFamilies.size(); i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    indices.transferFamily = i;
                    indices.transferFamilyHasValue = true;
                    transferQueueFamilyIndex_ = i;
                    break;
                }
            }
        }

        return indices;
    }

    SwapChainSupportDetails Device::querySwapChainSupport(const VkPhysicalDevice device) const {
        if (runningHeadless()) {
            throw std::runtime_error("No surface available in headless mode, cannot query SwapChain support");
        }

        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                surface_,
                &presentModeCount,
                details.presentModes.data());
        }
        return details;
    }

    VkFormat Device::findSupportedFormat(
        const std::vector<VkFormat> &candidates, const VkImageTiling tiling,
        const VkFormatFeatureFlags features) const {
        for (const VkFormat format: candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (
                tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }

    uint32_t Device::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }


    VkCommandBuffer Device::beginSingleTimeCommands() const {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = graphicsCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void Device::endSingleTimeCommands(const VkCommandBuffer commandBuffer) const {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue_);

        vkFreeCommandBuffers(device_, graphicsCommandPool, 1, &commandBuffer);
    }





    void Device::transitionImageLayout(const VkImage image,
                                       const VkImageLayout layoutOld,
                                       const VkImageLayout layoutNew,
                                       uint32_t layerCount,
                                       uint32_t baseLayer,
                                       uint32_t levelCount,
                                       uint32_t baseLevel,
                                       VkImageAspectFlags aspectMask) const {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = layoutOld;
        barrier.newLayout = layoutNew;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = baseLevel;
        barrier.subresourceRange.levelCount = levelCount;
        barrier.subresourceRange.baseArrayLayer = baseLayer;
        barrier.subresourceRange.layerCount = layerCount;

        // Set access masks and pipeline stages based on layout transition
        if (layoutOld == VK_IMAGE_LAYOUT_UNDEFINED && layoutNew == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_UNDEFINED && layoutNew ==
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_UNDEFINED && layoutNew ==
                   VK_IMAGE_LAYOUT_GENERAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_UNDEFINED && layoutNew ==
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            // Transition for a color attachment:
            // - No previous accesses.
            // - Destination will be written as a color attachment.
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            // The aspect mask is already set to VK_IMAGE_ASPECT_COLOR_BIT.
            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_UNDEFINED && layoutNew ==
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            // Transition for a depth/stencil attachment:
            // - No previous accesses.
            // - Destination will be read and written as a depth/stencil attachment.
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            // Change the aspect mask to depth (and optionally stencil).
            // If your format has a stencil component, you might OR in VK_IMAGE_ASPECT_STENCIL_BIT.
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_UNDEFINED && layoutNew ==
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layoutNew ==
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layoutNew ==
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layoutNew ==
                   VK_IMAGE_LAYOUT_GENERAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_HOST_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && layoutNew ==
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            // Transition from color attachment to shader read-only
            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layoutNew ==
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        endSingleTimeCommands(commandBuffer);
    }


    void Device::waitIdle() {
        vkDeviceWaitIdle(device());
    }
}
