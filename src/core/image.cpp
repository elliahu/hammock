#include "image.hpp"

#include <cstdint>
#include <vulkan/vulkan.hpp>


// ************* Image *********************
hammock::core::Image::Image(Device& device, const ImageDesc& desc) : BaseResource(device) {
    // Format and usage
    format_ = static_cast<vk::Format>(VulkanImageFormat{desc.format});
    usage_ = static_cast<vk::ImageUsageFlags>(VulkanImageUsage{desc.usage});
    type_ = static_cast<vk::ImageType>(VulkanImageType{desc.type});
    viewType_ = static_cast<vk::ImageViewType>(VulkanImageType{desc.type});

    // Image dimensions
    width_ = desc.width;
    height_ = desc.height;
    channels_ = desc.channels;
    depth_ = desc.depth;
    layers_ = desc.layers;
    mips_ = desc.mips;
    queueFamily_ = desc.advanced.currentQueueFamily;

    // attachment
    if (getAspectMask() & vk::ImageAspectFlagBits::eDepth) {
        clearValue_.depthStencil = vk::ClearDepthStencilValue{
            desc.depthStencilClearValue[0], static_cast<std::uint32_t>(desc.depthStencilClearValue[1])};
    } else {
        clearValue_.color = vk::ClearColorValue{desc.colorClearValue};
    }

    // queue family indices
    for (auto& family : desc.advanced.queueFamilies) {
        if (family == CommandQueueFamily::Graphics)
            queueFamilyIndices_.push_back(device.getGraphicsQueueFamilyIndex());
        if (family == CommandQueueFamily::Compute)
            queueFamilyIndices_.push_back(device.getComputeQueueFamilyIndex());
        if (family == CommandQueueFamily::Transfer)
            queueFamilyIndices_.push_back(device.getTransferQueueFamilyIndex());
    }

    sharingMode_ = desc.advanced.sharingMode;
    tiling_ = desc.advanced.tiling;
    memoryFlags_ = desc.advanced.memoryFlags;

    // Check for support
    vk::FormatProperties formatProperties = device.getPhysicalDevice().getFormatProperties(format_);

    assert(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);

    if (type_ == vk::ImageType::e3D) {
        uint32_t maxImageDimension3D(device.getPhysicalDeviceProperties().limits.maxImageDimension3D);
        assert(
            width_ <= maxImageDimension3D && height_ <= maxImageDimension3D && depth_ <= maxImageDimension3D);
    }

    /// Create tha gpu resource
    create();
}

hammock::core::Image::~Image() {
    if (resident) {
        Image::release();
    }
}

vk::RenderingAttachmentInfo hammock::core::Image::getRenderingAttachmentInfo(vk::ImageLayout layout) const {
    return {.imageView = view_, .imageLayout = layout, .clearValue = clearValue_};
}

vk::DescriptorImageInfo hammock::core::Image::getDescriptorImageInfo(vk::Sampler sampler) const {
    return {
        .sampler = sampler,
        .imageView = view_,
    };
}

vk::DescriptorImageInfo hammock::core::Image::getDescriptorImageInfo() const {
    if (sampler_ == nullptr) {
        throw std::runtime_error("sampler_ is nullptr");
    }
    return {
        .sampler = sampler_,
        .imageView = view_,
    };
}

vk::ImageAspectFlags hammock::core::Image::getAspectMask() const {
    switch (format_) {
        // Depth-only formats
        case vk::Format::eD16Unorm:
        case vk::Format::eD32Sfloat:
            return vk::ImageAspectFlagBits::eDepth;

        // Depth-stencil formats
        case vk::Format::eD16UnormS8Uint:
        case vk::Format::eD24UnormS8Uint:
        case vk::Format::eD32SfloatS8Uint:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

        // Everything else is treated as color
        default:
            return vk::ImageAspectFlagBits::eColor;
    }
}

vk::Sampler hammock::core::Image::createAndGetSampler() const {
    vk::Sampler sampler = nullptr;
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueWhite;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mips_);

    if (device.device().createSampler(&samplerInfo, nullptr, &sampler) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create sampler!");
    };

    return sampler;
}


vk::ImageSubresourceRange hammock::core::Image::getSubresourceRange(
    uint32_t baseMipLevel, uint32_t baseArrayLayer) const {
    return {.aspectMask = getAspectMask(),
        .baseMipLevel = baseMipLevel,
        .levelCount = mips_,
        .baseArrayLayer = baseArrayLayer,
        .layerCount = layers_};
}



void hammock::core::Image::create() {
    // Create the image
    vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.imageType = type_;
    imageCreateInfo.format = format_;
    imageCreateInfo.mipLevels = mips_;
    imageCreateInfo.arrayLayers = layers_;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.tiling = tiling_;
    imageCreateInfo.sharingMode = sharingMode_;
    imageCreateInfo.queueFamilyIndexCount = queueFamilyIndices_.size();
    imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices_.data();
    imageCreateInfo.extent.width = width_;
    imageCreateInfo.extent.height = height_;
    imageCreateInfo.extent.depth = depth_;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageCreateInfo.usage = usage_;

    allocator::AllocationCreateInfo allocInfo = {};
    allocInfo.usage = allocator::MemoryUsage::VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = static_cast<std::uint32_t>(memoryFlags_);  // FIXME

    try {
        allocator::createImage(
            device.allocator(), imageCreateInfo, &allocInfo, image_, &allocation_, nullptr);
    } catch (std::runtime_error& err) {
        throw;
    }

    // create image view
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = image_;
    viewInfo.viewType = viewType_;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = getAspectMask();
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mips_;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layers_;

    if (device.device().createImageView(&viewInfo, nullptr, &view_) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create image view");
    }

    resident = true;
}

void hammock::core::Image::release() {
    if (image_ != nullptr) {
        allocator::destroyImage(device.allocator(), image_, allocation_);
    }

    if (view_ != nullptr) {
        device.device().destroyImageView(view_, nullptr);
    }

    if (sampler_ != nullptr) {
        device.device().destroySampler(sampler_, nullptr);
    }

    resident = false;
}
