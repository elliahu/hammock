#include "image.hpp"

#include <compare>
#include <cstdint>
#include <vulkan/vulkan.hpp>


// ************* Image *********************
hammock::core::Image::Image(Device& device, uint64_t id, const ImageDesc& desc) : BaseResource(device, id) {
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
}

hammock::core::Image::~Image() {
    if (resident) {
        Image::release();
    }
}

vk::RenderingAttachmentInfo hammock::core::Image::getRenderingAttachmentInfo() const {
    return {.imageView = view_, .imageLayout = layout_, .clearValue = clearValue_};
}

vk::DescriptorImageInfo hammock::core::Image::getDescriptorImageInfo(vk::Sampler sampler) const {
    return {
        .sampler = sampler,
        .imageView = view_,
        .imageLayout = layout_,
    };
}

vk::DescriptorImageInfo hammock::core::Image::getDescriptorImageInfo() const {
    if (sampler_ == nullptr) {
        throw std::runtime_error("sampler_ is nullptr");
    }
    return {
        .sampler = sampler_,
        .imageView = view_,
        .imageLayout = layout_,
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

void hammock::core::Image::queueImageLayoutTransition(vk::ImageLayout newLayout) {
    if (newLayout == layout_) {
        return;
    }
    device.queueImageLayoutTransition(image_, layout_, newLayout, layers_, 0, mips_, 0, getAspectMask());
    layout_ = newLayout;
}

vk::ImageSubresourceRange hammock::core::Image::getSubresourceRange(
    uint32_t baseMipLevel, uint32_t baseArrayLayer) const {
    return {.aspectMask = getAspectMask(),
        .baseMipLevel = baseMipLevel,
        .levelCount = mips_,
        .baseArrayLayer = baseArrayLayer,
        .layerCount = layers_};
}

void hammock::core::Image::recordPipelineBarrier(vk::CommandBuffer cmd, vk::PipelineStageFlags2 srcStageMask,
    vk::AccessFlags2 srcAccessMask, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t srcQueueFamilyIndex,
    uint32_t dstQueueFamilyIndex) {
    // Subresource range
    vk::ImageSubresourceRange subresourceRange = getSubresourceRange();

    vk::ImageMemoryBarrier2 imageBarrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,  // Optional: layout transition
        .srcQueueFamilyIndex = srcQueueFamilyIndex,
        .dstQueueFamilyIndex = dstQueueFamilyIndex,
        .image = image_,
        .subresourceRange = subresourceRange,
    };

    vk::DependencyInfo depInfo = {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    cmd.pipelineBarrier2(&depInfo);

    layout_ = newLayout;
}

void hammock::core::Image::queueCopyFromBuffer(Buffer& buffer) const {
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;    // tightly packed
    region.bufferImageHeight = 0;  // tightly packed

    region.imageSubresource.aspectMask = getAspectMask();
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layers_;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width_, height_, depth_};

    auto cmd = device.beginSingleTimeCommands();
    cmd.copyBufferToImage(buffer.getBuffer(), image_, vk::ImageLayout::eTransferDstOptimal, 1, &region);
    device.endSingleTimeCommands(cmd);
}

void hammock::core::Image::queueCopyFromImage(Image& image) const {
    vk::ImageCopy region{};
    region.srcSubresource.aspectMask = image.getAspectMask();
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = image.getLayerLevel();

    region.srcOffset = {0, 0, 0};

    region.dstSubresource.aspectMask = getAspectMask();
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = layers_;

    region.dstOffset = {0, 0, 0};
    region.extent = {width_, height_, depth_};

    auto cmd = device.beginSingleTimeCommands();
    cmd.copyImage(image.getImage(),
        vk::ImageLayout::eTransferSrcOptimal,
        image_,
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &region);
    device.endSingleTimeCommands(cmd);
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

void hammock::core::Image::generateMips() {
    vk::CommandBuffer commandBuffer = device.beginSingleTimeCommands();

    recordPipelineBarrier(commandBuffer,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored);

    vk::ImageMemoryBarrier barrier{};
    barrier.image = image_;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange.aspectMask = getAspectMask();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    auto mipWidth = static_cast<int32_t>(width_);
    auto mipHeight = static_cast<int32_t>(height_);

    for (uint32_t i = 1; i < mips_; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = getAspectMask();
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = getAspectMask();
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffer.blitImage(image_,
            vk::ImageLayout::eTransferSrcOptimal,
            image_,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &blit,
            vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mips_ - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    layout_ = vk::ImageLayout::eShaderReadOnlyOptimal;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    device.endSingleTimeCommands(commandBuffer);
    device.waitIdle();
}

// **************** Sampler **********************

hammock::core::Sampler::Sampler(Device& device, uint64_t id, const SamplerDesc& desc)
    : BaseResource(device, id) {
    magFilter_ = desc.magFilter;
    minFilter_ = desc.minFilter;
    addressModeU_ = desc.addressModeU;
    addressModeV_ = desc.addressModeV;
    addressModeW_ = desc.addressModeW;
    anisotropyEnable_ = desc.anisotropyEnable;
    borderColor_ = desc.borderColor;
    mipmapMode_ = desc.mipmapMode;
    mips_ = desc.mips;
    mipLodBias_ = desc.mipLodBias;

    // retrieve max anisotropy from physical device
    maxAnisotropy_ = device.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
}

hammock::core::Sampler::~Sampler() {
    if (resident) {
        Sampler::release();
    }
}

void hammock::core::Sampler::create() {
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = magFilter_;
    samplerInfo.minFilter = minFilter_;
    samplerInfo.addressModeU = addressModeU_;
    samplerInfo.addressModeV = addressModeV_;
    samplerInfo.addressModeW = addressModeW_;
    samplerInfo.anisotropyEnable = anisotropyEnable_;
    samplerInfo.maxAnisotropy = maxAnisotropy_;
    samplerInfo.borderColor = borderColor_;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = mipmapMode_;
    samplerInfo.mipLodBias = mipLodBias_;
    samplerInfo.minLod = 0.0;
    samplerInfo.maxLod = mips_;

    if (device.device().createSampler(&samplerInfo, nullptr, &sampler_) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create sampler");
    }
    resident = true;
}

void hammock::core::Sampler::release() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device.device(), sampler_, nullptr);
    }
    resident = false;
}
