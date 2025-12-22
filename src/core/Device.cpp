module;
#include <stdexcept>
#include <set>
#include <vector>


module hammock.core.device;

import vulkan_hpp;
import hammock.core.utilities;
import hammock.core.memory_allocator;

namespace hammock::core {
    // class member functions
    Device::Device(Instance &instance, vk::SurfaceKHR surface) : instance{instance}, surface_{surface} {
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPools();
        createMemoryAllocator();
    }

    Device::~Device() {
        allocator::destroyAllocator(allocator_);
        device_.destroyCommandPool(graphicsCommandPool);
        device_.destroyCommandPool(transferCommandPool);
        device_.destroyCommandPool(computeCommandPool);
        device_.destroy();
    }




    void Device::pickPhysicalDevice() {
        std::vector<vk::PhysicalDevice> devices = instance.getInstance().enumeratePhysicalDevices();

        for (const auto &device: devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (!physicalDevice) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        physicalDeviceProperties = physicalDevice.getProperties();
        Logger::info("Physical device: %s", physicalDeviceProperties.deviceName);

        if (runningHeadless()) {
            Logger::info("Running in headless mode");
        }
    }

    void Device::createLogicalDevice() {
        auto queFamilyIndices = findQueueFamilies(
            physicalDevice);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            queFamilyIndices.graphicsFamily, queFamilyIndices.presentFamily, queFamilyIndices.transferFamily,
            queFamilyIndices.computeFamily
        };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = vk::True;
        deviceFeatures.fillModeNonSolid = vk::True;

        // Create the physical device features structures

        vk::PhysicalDeviceSynchronization2FeaturesKHR sync2Features{};
        sync2Features.synchronization2 = vk::True;

        vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
        // Enable non-uniform indexing
        descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = vk::True;
        descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = vk::True;
        descriptorIndexingFeatures.runtimeDescriptorArray = vk::True;
        descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = vk::True;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound = vk::True;
        descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = vk::True;
        descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = vk::True;
        descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = vk::True;
        descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = vk::True;

        vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
        dynamicRenderingFeatures.dynamicRendering = vk::True;

        // Chain the features structures
        descriptorIndexingFeatures.pNext = &dynamicRenderingFeatures;
        dynamicRenderingFeatures.pNext = &sync2Features;

        // Populate VkPhysicalDeviceFeatures2
        vk::PhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.pNext = &descriptorIndexingFeatures;
        deviceFeatures2.features = deviceFeatures;

        // Include deviceFeatures2 in VkDeviceCreateInfo pNext field
        vk::DeviceCreateInfo createInfo = {};
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr;
        // Ensure no enabled features from previous Vulkan versions are left active
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pNext = &deviceFeatures2; // Pass the device features structure
        createInfo.enabledLayerCount = 0;


        if (physicalDevice.createDevice(&createInfo, nullptr, &device_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create logical device!");
        }

        device_.getQueue(queFamilyIndices.graphicsFamily, 0, &graphicsQueue_);
        device_.getQueue(queFamilyIndices.presentFamily, 0, &presentQueue_);
        device_.getQueue(queFamilyIndices.transferFamily, 0, &transferQueue_);
        device_.getQueue(queFamilyIndices.computeFamily, 0, &computeQueue_);
    }

    void Device::createCommandPools() {
        auto queFamilyIndices =
                findPhysicalQueueFamilies();

        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.queueFamilyIndex = queFamilyIndices.graphicsFamily;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;



        if (device_.createCommandPool(&poolInfo, nullptr, &graphicsCommandPool) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

        poolInfo.queueFamilyIndex = queFamilyIndices.transferFamily;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

        if (device_.createCommandPool(&poolInfo, nullptr, &transferCommandPool) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create transfer command pool!");
        }

        poolInfo.queueFamilyIndex = queFamilyIndices.computeFamily;

        if (device_.createCommandPool(&poolInfo, nullptr, &computeCommandPool) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create compute command pool!");
        }
    }

    void Device::createMemoryAllocator() {

        allocator::AllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = allocator::AllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = vk::makeApiVersion(0, 1,3,0);
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device_;
        allocatorCreateInfo.instance = instance.getInstance();

        try {
            allocator::createAllocator(&allocatorCreateInfo, &allocator_);
        } catch (std::runtime_error &e) {
            throw;
        }
    }

    bool Device::isDeviceSuitable(vk::PhysicalDevice device) {
        const QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = runningHeadless();
        if (extensionsSupported && !runningHeadless()) {
            const auto [capabilities, formats, presentModes] = querySwapChainSupport(device);
            swapChainAdequate = !formats.empty() && !presentModes.empty();
        }

        vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

        return indices.isComplete() && extensionsSupported && swapChainAdequate &&
               supportedFeatures.samplerAnisotropy;
    }


    bool Device::checkDeviceExtensionSupport(const vk::PhysicalDevice device) const {
        std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension: availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices Device::findQueueFamilies(const vk::PhysicalDevice device) {
        QueueFamilyIndices indices;
        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            if (queueFamily.queueCount > 0) {
                // Graphics queue
                if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                    indices.graphicsFamily = i;
                    indices.graphicsFamilyHasValue = true;
                    graphicsQueueFamilyIndex_ = i;
                }

                // Only ask for present queue when using surface
                if (!runningHeadless()) {
                    vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface_);
                    if (presentSupport) {
                        indices.presentFamily = i;
                        indices.presentFamilyHasValue = true;
                    }
                }


                // Compute queue (preferably separate from graphics)
                if ((queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
                    indices.computeFamily = i;
                    indices.computeFamilyHasValue = true;
                    computeQueueFamilyIndex_ = i;
                }

                // Transfer queue (preferably separate from graphics and compute)
                if ((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
                    !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
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
                if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
                    indices.computeFamily = i;
                    indices.computeFamilyHasValue = true;
                    computeQueueFamilyIndex_ = i;
                    break;
                }
            }
        }

        if (!indices.transferFamilyHasValue) {
            for (i = 0; i < queueFamilies.size(); i++) {
                if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) {
                    indices.transferFamily = i;
                    indices.transferFamilyHasValue = true;
                    transferQueueFamilyIndex_ = i;
                    break;
                }
            }
        }

        return indices;
    }

    SwapChainSupportDetails Device::querySwapChainSupport(const vk::PhysicalDevice device) const {
        if (runningHeadless()) {
            throw std::runtime_error("No surface available in headless mode, cannot query SwapChain support");
        }

        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface_);
        details.formats = device.getSurfaceFormatsKHR(surface_);
        details.presentModes = device.getSurfacePresentModesKHR(surface_);
        return details;
    }

    vk::Format Device::findSupportedFormat(
        const std::vector<vk::Format> &candidates, const vk::ImageTiling tiling,
        const vk::FormatFeatureFlags features) const {
        for (const vk::Format format: candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }

            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }

    uint32_t Device::findMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags properties) const {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }


    vk::CommandBuffer Device::beginSingleTimeCommands() const {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = graphicsCommandPool;
        allocInfo.commandBufferCount = 1;

        vk::CommandBuffer commandBuffer;
        if (device_.allocateCommandBuffers(&allocInfo, &commandBuffer) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to begin command buffer!");
        }
        return commandBuffer;
    }

    void Device::endSingleTimeCommands(const vk::CommandBuffer commandBuffer) const {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (graphicsQueue_.submit(1, &submitInfo, nullptr) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to submit command buffer!");
        }

        graphicsQueue_.waitIdle();
        device_.freeCommandBuffers(graphicsCommandPool, 1, &commandBuffer);
    }

    void Device::transitionImageLayout(const vk::Image image,
                                       const vk::ImageLayout layoutOld,
                                       const vk::ImageLayout layoutNew,
                                       uint32_t layerCount,
                                       uint32_t baseLayer,
                                       uint32_t levelCount,
                                       uint32_t baseLevel,
                                       vk::ImageAspectFlags aspectMask) const {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = layoutOld;
        barrier.newLayout = layoutNew;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = baseLevel;
        barrier.subresourceRange.levelCount = levelCount;
        barrier.subresourceRange.baseArrayLayer = baseLayer;
        barrier.subresourceRange.layerCount = layerCount;

        // Set access masks and pipeline stages based on layout transition
        if (layoutOld == vk::ImageLayout::eUndefined && layoutNew == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            vk::PipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            vk::PipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            CmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == vk::ImageLayout::eUndefined && layoutNew ==
                   vk::ImageLayout::eTransferSrcOptimal) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vk::PipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            vk::PipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == vk::ImageLayout::eUndefined && layoutNew ==
                   vk::ImageLayout::eGeneral) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

            vk::PipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            vk::PipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == vk::ImageLayout::eUndefined && layoutNew ==
                   vk::ImageLayout::eColorAttachmentOptimal) {
            // Transition for a color attachment:
            // - No previous accesses.
            // - Destination will be written as a color attachment.
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            // The aspect mask is already set to VK_IMAGE_ASPECT_COLOR_BIT.
            VkPipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == vk::ImageLayout::eUndefined && layoutNew ==
                   vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            // Transition for a depth/stencil attachment:
            // - No previous accesses.
            // - Destination will be read and written as a depth/stencil attachment.
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            // Change the aspect mask to depth (and optionally stencil).
            // If your format has a stencil component, you might OR in VK_IMAGE_ASPECT_STENCIL_BIT.
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            VkPipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == vk::ImageLayout::eUndefined && layoutNew ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            VkPipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 sourceStage, destinationStage,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        } else if (layoutOld == vk::ImageLayout::eTransferDstOptimal && layoutNew ==
                   vk::ImageLayout::eColorAttachmentOptimal) {
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
        } else if (layoutOld == vk::ImageLayout::eTransferDstOptimal && layoutNew ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
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
        } else if (layoutOld == vk::ImageLayout::eTransferDstOptimal && layoutNew ==
                   vk::ImageLayout::eGeneral) {
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
        } else if (layoutOld == vk::ImageLayout::eColorAttachmentOptimal && layoutNew ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
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
        } else if (layoutOld == vk::ImageLayout::eTransferDstOptimal && layoutNew ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
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
