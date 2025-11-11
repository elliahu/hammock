#pragma once

#include "hammock/core/Device.h"
#include "hammock/utils/Singleton.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace Hammock {
    typedef const uint32_t Binding;

    typedef VkDescriptorSet DescriptorSet;

    class DescriptorSetLayout {
       public:
        class Builder {
           public:
            explicit Builder(Device& device) : device{device} {}

            Builder& addBinding(uint32_t binding, VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags, uint32_t count = 1, VkDescriptorBindingFlags flags = 0);

            std::unique_ptr<DescriptorSetLayout> build() const;

           private:
            Device& device;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags{};
        };

        DescriptorSetLayout(Device& device,
            const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings,
            const std::unordered_map<uint32_t, VkDescriptorBindingFlags>& flags);

        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout&) = delete;

        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

       private:
        Device& device;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class DescriptorWriter;
    };

    class DescriptorPool : public Singleton<DescriptorPool> {
        friend class Singleton<DescriptorPool>;
        friend class DescriptorWriter;

       public:
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        ~DescriptorPool();

        static void initialize(Device& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes) {
            Singleton<DescriptorPool>::initialize(device, maxSets, poolFlags, poolSizes);
        }

        bool allocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(const std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool() const;

        VkDescriptorPool descriptorPool;

       protected:
        DescriptorPool(Device& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);

        Device& device;
    };

    class DescriptorWriter {
       public:
        DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

        DescriptorWriter& writeBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo);

        DescriptorWriter& writeBufferArray(
            uint32_t binding, const std::vector<VkDescriptorBufferInfo>& bufferInfos);

        DescriptorWriter& writeImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo);

        DescriptorWriter& writeImageArray(
            uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfos);

        DescriptorWriter& writeAccelerationStructure(
            uint32_t binding, const VkWriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo);

        bool build(VkDescriptorSet& set);

        void overwrite(VkDescriptorSet& set);

       private:
        DescriptorSetLayout& setLayout;
        DescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

    struct DescriptorSetsAndLayout {
        std::vector<DescriptorSet> sets{};
        std::unique_ptr<DescriptorSetLayout> layout{nullptr};
    };
}  // namespace Hammock
