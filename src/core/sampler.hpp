#pragma once

#include <cassert>
#include <cstdint>

#include "base_resource.hpp"
#include "device.hpp"
#include "vulkan/vulkan.hpp"

namespace hammock::core{

    /// @struct SamplerDesc
    /// @brief Describes a vulkan sampler
    struct SamplerDesc {
        vk::Filter magFilter = vk::Filter::eLinear;
        vk::Filter minFilter = vk::Filter::eLinear;
        vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
        vk::Bool32 anisotropyEnable = true;
        vk::BorderColor borderColor = vk::BorderColor::eIntOpaqueBlack;
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
        std::uint32_t mips = 1;
        float mipLodBias = 0.0f;
    };

    /// @class Sampler
    /// @brief Wrapper around vulkan sampler handle
    class Sampler : public BaseResource {
        vk::Sampler sampler_ = nullptr;
        vk::Filter magFilter_;
        vk::Filter minFilter_;
        vk::SamplerAddressMode addressModeU_;
        vk::SamplerAddressMode addressModeV_;
        vk::SamplerAddressMode addressModeW_;
        vk::Bool32 anisotropyEnable_;
        float maxAnisotropy_;
        vk::BorderColor borderColor_;
        vk::SamplerMipmapMode mipmapMode_;
        std::uint32_t mips_;
        float mipLodBias_;

       public:
        Sampler(Device& device,const SamplerDesc& desc);

        ~Sampler() override;

        void create() override;

        void release() override;

        /// @brief Get the vulkan sampler handle
        [[nodiscard]] vk::Sampler getSampler() const { return sampler_; }
    };
}
