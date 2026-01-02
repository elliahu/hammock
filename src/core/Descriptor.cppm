module;
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>


export module hammock.core.descriptor;

import hammock.core.device;
import hammock.core.utilities;




namespace hammock::core {
    export typedef const uint32_t Binding;

    export typedef vk::DescriptorSet DescriptorSet;

    export class DescriptorSetLayout;
    export class DescriptorPool;
    export class DescriptorWriter;

    class DescriptorSetLayout {
       public:
        class Builder {
           public:
            explicit Builder(Device& device) : device{device} {}

            Builder& addBinding(uint32_t binding, vk::DescriptorType descriptorType,
                vk::ShaderStageFlags stageFlags, uint32_t count = 1, vk::DescriptorBindingFlags flags = {});

            std::unique_ptr<DescriptorSetLayout> build() const;

           private:
            Device& device;
            std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings{};
            std::unordered_map<uint32_t, vk::DescriptorBindingFlags> bindingFlags{};
        };

        DescriptorSetLayout(Device& device,
            const std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding>& bindings,
            const std::unordered_map<uint32_t, vk::DescriptorBindingFlags>& flags);

        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout&) = delete;

        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        vk::DescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

       private:
        Device& device;
        vk::DescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings;

        friend class DescriptorWriter;
    };

    class DescriptorPool : public Singleton<DescriptorPool> {
        friend class Singleton<DescriptorPool>;
        friend class DescriptorWriter;

       public:
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        ~DescriptorPool();

        static void initialize(Device& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags,
            const std::vector<vk::DescriptorPoolSize>& poolSizes) {
            Singleton<DescriptorPool>::initialize(device, maxSets, poolFlags, poolSizes);
        }

        bool allocateDescriptor(vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet& descriptor) const;

        void freeDescriptors(const std::vector<vk::DescriptorSet>& descriptors) const;

        void resetPool() const;

        vk::DescriptorPool descriptorPool;

       protected:
        DescriptorPool(Device& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags,
            const std::vector<vk::DescriptorPoolSize>& poolSizes);

        Device& device;
    };

    class DescriptorWriter {
       public:
        DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

        DescriptorWriter& writeBuffer(uint32_t binding, const vk::DescriptorBufferInfo* bufferInfo);

        DescriptorWriter& writeBufferArray(
            uint32_t binding, const std::vector<vk::DescriptorBufferInfo>& bufferInfos);

        DescriptorWriter& writeImage(uint32_t binding, const vk::DescriptorImageInfo* imageInfo);

        DescriptorWriter& writeImageArray(
            uint32_t binding, const std::vector<vk::DescriptorImageInfo>& imageInfos);

        DescriptorWriter& writeAccelerationStructure(
            uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo);

        bool build(vk::DescriptorSet& set);

        void overwrite(vk::DescriptorSet& set);

       private:
        DescriptorSetLayout& setLayout;
        DescriptorPool& pool;
        std::vector<vk::WriteDescriptorSet> writes;
    };

    export struct DescriptorSetsAndLayout {
        std::vector<DescriptorSet> sets{};
        std::unique_ptr<DescriptorSetLayout> layout{nullptr};
    };
}  // namespace hammock::core
