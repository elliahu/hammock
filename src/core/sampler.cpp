#include "sampler.hpp"


hammock::core::Sampler::Sampler(Device& device, const SamplerDesc& desc)
    : BaseResource(device) {
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

    create();
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
