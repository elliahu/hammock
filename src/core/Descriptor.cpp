module;
#include <memory>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

module hammock.core.descriptor;



namespace hammock::core {
    // *************** Descriptor Set Layout Builder *********************

    DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::addBinding(
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
        bindings[binding] = layoutBinding;
        bindingFlags[binding] = flags;
        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const {
        return std::make_unique<DescriptorSetLayout>(device, bindings, bindingFlags);
    }

    // *************** Descriptor Set Layout *********************

    DescriptorSetLayout::DescriptorSetLayout(
        Device &device, const std::unordered_map<uint32_t,
            vk::DescriptorSetLayoutBinding> &bindings,
        const std::unordered_map<uint32_t, vk::DescriptorBindingFlags> &flags)
        : device{device}, bindings{bindings} {
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
                &descriptorSetLayout) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        device.device().destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
    }

    // *************** Descriptor Pool *********************

    DescriptorPool::DescriptorPool(
        Device &device,
        const uint32_t maxSets,
        const vk::DescriptorPoolCreateFlags poolFlags,
        const std::vector<vk::DescriptorPoolSize> &poolSizes)
        : device{device} {
        vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (device.device().createDescriptorPool(&descriptorPoolInfo, nullptr, &descriptorPool) !=
            vk::Result::eSuccess) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        device.device().destroyDescriptorPool(descriptorPool, nullptr);
    }

    bool DescriptorPool::allocateDescriptor(
        const vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet &descriptor) const {
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up. But this is beyond our current scope
        if (device.device().allocateDescriptorSets(&allocInfo, &descriptor) != vk::Result::eSuccess) {
            return false;
        }
        return true;
    }

    void DescriptorPool::freeDescriptors(const std::vector<vk::DescriptorSet> &descriptors) const {
        device.device().freeDescriptorSets(
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    void DescriptorPool::resetPool() const {
        device.device().resetDescriptorPool(descriptorPool);
    }

    // *************** Descriptor Writer *********************

    DescriptorWriter::DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
        : setLayout{setLayout}, pool{pool} {
    }

    DescriptorWriter &DescriptorWriter::writeBuffer(
        const uint32_t binding, const vk::DescriptorBufferInfo *bufferInfo) {
        if (setLayout.bindings.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }


        const auto &[_binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers] = setLayout.bindings[
            binding];

        if (descriptorCount != 1) {
            throw std::runtime_error("Binding single descriptor info, but binding expects multiple");
        }

        vk::WriteDescriptorSet write{};
        write.descriptorType = descriptorType;
        write.dstBinding = _binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeBufferArray(const uint32_t binding,
                                                         const std::vector<vk::DescriptorBufferInfo> &bufferInfos) {
        if (setLayout.bindings.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &bindingDescription = setLayout.bindings[binding];

        vk::WriteDescriptorSet write{};
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfos.data();
        write.descriptorCount = static_cast<uint32_t>(bufferInfos.size());

        writes.push_back(write);
        return *this;
    }


    DescriptorWriter &DescriptorWriter::writeImage(
        uint32_t binding, const vk::DescriptorImageInfo *imageInfo) {
        if (setLayout.bindings.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &bindingDescription = setLayout.bindings[binding];

        if (bindingDescription.descriptorCount != 1) {
            throw std::runtime_error("Binding single descriptor info, but binding expects multiple");
        }

        vk::WriteDescriptorSet write{};
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeImageArray(const uint32_t binding,
                                                        const std::vector<vk::DescriptorImageInfo> &imageInfos) {
        if (setLayout.bindings.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &[setLayoutBinding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers] = setLayout.bindings[
            binding];


        vk::WriteDescriptorSet write{};
        write.descriptorType = descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfos.data();
        write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        if (!imageInfos.empty())
            writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeAccelerationStructure(const uint32_t binding,
                                                                   const vk::WriteDescriptorSetAccelerationStructureKHR *
                                                                   accelerationStructureInfo) {
        if (setLayout.bindings.count(binding) != 1) {
            throw std::runtime_error("Layout does not contain specified binding");
        }

        auto &bindingDescription = setLayout.bindings[binding];

        if (bindingDescription.descriptorCount != 1) {
            throw std::runtime_error("Binding single descriptor info, but binding expects multiple");
        }

        vk::WriteDescriptorSet write{};
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.descriptorCount = 1;
        write.pNext = accelerationStructureInfo;

        writes.push_back(write);
        return *this;
    }

    bool DescriptorWriter::build(vk::DescriptorSet &set) {
        if (const bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set); !success) {
            throw std::runtime_error("Failed to allocate descriptors from the pool!");
        }
        overwrite(set);
        return true;
    }

    void DescriptorWriter::overwrite(vk::DescriptorSet &set) {
        for (auto &write: writes) {
            write.dstSet = set;
        }
        pool.device.device().updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
    }
} // namespace hmck
