#pragma once
#include <memory>
#include <unordered_map>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "utilities.hpp"




namespace hammock::core {
    typedef const uint32_t Binding;

    typedef vk::DescriptorSet DescriptorSet;

    class DescriptorSetLayout;
    class DescriptorPool;
    class DescriptorWriter;

    /// @class DescriptorSetLayout
    /// @brief Wrapper class around Vulkan descriptor set layout which describes what kind of resources are bound
    class DescriptorSetLayout {
       public:
        DescriptorSetLayout(Device& device,
            const std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding>& bindings,
            const std::unordered_map<uint32_t, vk::DescriptorBindingFlags>& flags);

        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout&) = delete;

        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        /// @brief Get Vulkan descriptor set layout handle
        [[nodiscard]] vk::DescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_; }

       private:
        Device& device_;
        vk::DescriptorSetLayout descriptorSetLayout_;
        std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings_;

        friend class DescriptorWriter;
    };

    /// @class DescriptorSetLayoutBuilder
    /// @brief Builder style helper for creating descriptor set layouts
    class DescriptorSetLayoutBuilder {
    public:
        explicit DescriptorSetLayoutBuilder(Device& device) : device_{device} {}

        /// @brief Add binding to the layout
        DescriptorSetLayoutBuilder& addBinding(uint32_t binding, vk::DescriptorType descriptorType,
            vk::ShaderStageFlags stageFlags, uint32_t count = 1, vk::DescriptorBindingFlags flags = {});

        /// @brief Build the actual layout as a unique ptr
        [[nodiscard]] std::unique_ptr<DescriptorSetLayout> build() const;

    private:
        Device& device_;
        std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings_{};
        std::unordered_map<uint32_t, vk::DescriptorBindingFlags> bindingFlags_{};
    };

    /// @class DescriptorPool
    /// @brief Wrapper class around Vulkan descriptor pool.
    /// Used to allocate descriptors. Singleton class.
    class DescriptorPool final : public Singleton<DescriptorPool> {
        friend class Singleton<DescriptorPool>;
        friend class DescriptorWriter;

       public:
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        ~DescriptorPool() override;

        /// @brief Initialize the singleton instance
        static void initialize(Device& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags,
            const std::vector<vk::DescriptorPoolSize>& poolSizes);

        /// @brief Allocate descriptor from the pool
        bool allocateDescriptor(vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet& descriptor) const;

        /// @brief Free allocated descriptor
        void freeDescriptors(const std::vector<vk::DescriptorSet>& descriptors) const;

        /// @brief Reset the pool
        void resetPool() const;

        /// @brief Get the Vulkan handle
        vk::DescriptorPool& getDescriptorPool() {return descriptorPool_;}

       protected:
        DescriptorPool(Device& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags,
            const std::vector<vk::DescriptorPoolSize>& poolSizes);

        Device& device_;
        vk::DescriptorPool descriptorPool_;
    };

    /// @class DescriptorWriter
    /// Helper class for writing data into descriptors.
    /// Given a descriptor set layout, using this builder style class you can create a concrete descriptor set.
    class DescriptorWriter {
       public:
        DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

        /// @brief Write buffer to specific binding slot
        DescriptorWriter& writeBuffer(uint32_t binding, const vk::DescriptorBufferInfo* bufferInfo);

        /// @brief Write buffer array
        DescriptorWriter& writeBufferArray(
            uint32_t binding, const std::vector<vk::DescriptorBufferInfo>& bufferInfos);

        /// @brief Write image
        DescriptorWriter& writeImage(uint32_t binding, const vk::DescriptorImageInfo* imageInfo);

        /// @brief Write image array
        DescriptorWriter& writeImageArray(
            uint32_t binding, const std::vector<vk::DescriptorImageInfo>& imageInfos);

        DescriptorWriter& writeAccelerationStructure(
            uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo);

        /// @brief Build the actual descriptor set
        bool build(vk::DescriptorSet& set);

        /// @brief Overwrite the set data
        void overwrite(vk::DescriptorSet& set);

       private:
        DescriptorSetLayout& setLayout_;
        DescriptorPool& pool_;
        std::vector<vk::WriteDescriptorSet> writes_;
    };
}  // namespace hammock::core
