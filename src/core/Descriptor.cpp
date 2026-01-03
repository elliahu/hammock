module;
#include <memory>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.core.descriptor;



namespace hammock::core {
    // *************** Descriptor Set Layout Builder *********************

    DescriptorSetLayoutBuilder &DescriptorSetLayoutBuilder::addBinding(
        const uint32_t binding,
        const vk::DescriptorType descriptorType,
        const vk::ShaderStageFlags stageFlags,
        const uint32_t count,
        const vk::DescriptorBindingFlags flags) {
        vk::DescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings_[binding] = layoutBinding;
        bindingFlags_[binding] = flags;
        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayoutBuilder::build() const {
        return std::make_unique<DescriptorSetLayout>(device_, bindings_, bindingFlags_);
    }

    // *************** Descriptor Set Layout *********************

    DescriptorSetLayout::DescriptorSetLayout(
        Device &device, const std::unordered_map<uint32_t,
            vk::DescriptorSetLayoutBinding> &bindings,
        const std::unordered_map<uint32_t, vk::DescriptorBindingFlags> &flags)
        : device_{device}, bindings_{bindings} {
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{};
        std::vector<vk::DescriptorBindingFlags> setLayoutBindingFlags{};
        for (auto [fst, snd]: bindings) {
            setLayoutBindings.push_back(snd);
        }

        for (auto [fst, snd]: flags) {
            setLayoutBindingFlags.push_back(snd);
        }

        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
        bindingFlagsInfo.bindingCount = static_cast<uint32_t>(setLayoutBindingFlags.size());
        bindingFlagsInfo.pBindingFlags = setLayoutBindingFlags.data();

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();
        descriptorSetLayoutInfo.pNext = &bindingFlagsInfo;
        descriptorSetLayoutInfo.flags = {}; //VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;


        if (device.device().createDescriptorSetLayout(
                &descriptorSetLayoutInfo,
                nullptr,
                &descriptorSetLayout_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        device_.device().destroyDescriptorSetLayout(descriptorSetLayout_, nullptr);
    }

    // *************** Descriptor Pool *********************

    DescriptorPool::DescriptorPool(
        Device &device,
        const uint32_t maxSets,
        const vk::DescriptorPoolCreateFlags poolFlags,
        const std::vector<vk::DescriptorPoolSize> &poolSizes)
        : device_{device} {
        vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (device.device().createDescriptorPool(&descriptorPoolInfo, nullptr, &descriptorPool_) !=
            vk::Result::eSuccess) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        device_.device().destroyDescriptorPool(descriptorPool_, nullptr);
    }

    void DescriptorPool::initialize(Device &device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags,
        const std::vector<vk::DescriptorPoolSize> &poolSizes) {
        Singleton<DescriptorPool>::initialize(device, maxSets, poolFlags, poolSizes);
    }

    bool DescriptorPool::allocateDescriptor(
        const vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet &descriptor) const {
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up. But this is beyond our current scope
        if (device_.device().allocateDescriptorSets(&allocInfo, &descriptor) != vk::Result::eSuccess) {
            return false;
        }
        return true;
    }

    void DescriptorPool::freeDescriptors(const std::vector<vk::DescriptorSet> &descriptors) const {
        device_.device().freeDescriptorSets(
            descriptorPool_,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    void DescriptorPool::resetPool() const {
        device_.device().resetDescriptorPool(descriptorPool_);
    }

    // *************** Descriptor Writer *********************

    DescriptorWriter::DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
        : setLayout_{setLayout}, pool_{pool} {
    }

    DescriptorWriter &DescriptorWriter::writeBuffer(
        const uint32_t binding, const vk::DescriptorBufferInfo *bufferInfo) {
        if (setLayout_.bindings_.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }


        const auto &[_binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers] = setLayout_.bindings_[
            binding];

        if (descriptorCount != 1) {
            throw std::runtime_error("Binding single descriptor info, but binding expects multiple");
        }

        vk::WriteDescriptorSet write{};
        write.descriptorType = descriptorType;
        write.dstBinding = _binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes_.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeBufferArray(const uint32_t binding,
                                                         const std::vector<vk::DescriptorBufferInfo> &bufferInfos) {
        if (setLayout_.bindings_.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &bindingDescription = setLayout_.bindings_[binding];

        vk::WriteDescriptorSet write{};
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfos.data();
        write.descriptorCount = static_cast<uint32_t>(bufferInfos.size());

        writes_.push_back(write);
        return *this;
    }


    DescriptorWriter &DescriptorWriter::writeImage(
        uint32_t binding, const vk::DescriptorImageInfo *imageInfo) {
        if (setLayout_.bindings_.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &bindingDescription = setLayout_.bindings_[binding];

        if (bindingDescription.descriptorCount != 1) {
            throw std::runtime_error("Binding single descriptor info, but binding expects multiple");
        }

        vk::WriteDescriptorSet write{};
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes_.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeImageArray(const uint32_t binding,
                                                        const std::vector<vk::DescriptorImageInfo> &imageInfos) {
        if (setLayout_.bindings_.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &[setLayoutBinding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers] = setLayout_.bindings_[
            binding];


        vk::WriteDescriptorSet write{};
        write.descriptorType = descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfos.data();
        write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        if (!imageInfos.empty())
            writes_.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeAccelerationStructure(const uint32_t binding,
                                                                   const vk::WriteDescriptorSetAccelerationStructureKHR *
                                                                   accelerationStructureInfo) {
        if (setLayout_.bindings_.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &bindingDescription = setLayout_.bindings_[binding];

        if (bindingDescription.descriptorCount != 1) {
            throw std::runtime_error("Binding single descriptor info, but binding expects multiple");
        }

        vk::WriteDescriptorSet write{};
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.descriptorCount = 1;
        write.pNext = accelerationStructureInfo;

        writes_.push_back(write);
        return *this;
    }

    bool DescriptorWriter::build(vk::DescriptorSet &set) {
        if (const bool success = pool_.allocateDescriptor(setLayout_.getDescriptorSetLayout(), set); !success) {
            throw std::runtime_error("Failed to allocate descriptors from the pool!");
        }
        overwrite(set);
        return true;
    }

    void DescriptorWriter::overwrite(vk::DescriptorSet &set) {
        for (auto &write: writes_) {
            write.dstSet = set;
        }
        pool_.device_.device().updateDescriptorSets(writes_.size(), writes_.data(), 0, nullptr);
    }
} // namespace hmck
