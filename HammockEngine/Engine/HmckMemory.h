#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <functional>

#include <HmckBuffer.h>
#include <HmckDescriptors.h>

namespace Hmck
{
	typedef uint32_t UniformBufferHandle;
	typedef uint32_t DescriptorSetHandle;
	typedef uint32_t DescriptorSetLayoutHandle;

	class DescriptorManager
	{
	public:

		DescriptorManager(Device& device);

		struct UniformBufferCreateInfo
		{
			Device& device;
			VkDeviceSize instanceSize;
			uint32_t instanceCount;
			VkBufferUsageFlags usageFlags;
			VkMemoryPropertyFlags memoryPropertyFlags;
			VkDeviceSize minOffsetAlignment = 1;
		};

		UniformBufferHandle createUniformBuffer(UniformBufferCreateInfo createInfo);

		struct DescriptorSetLayoutCreateInfo
		{
			

			struct DescriptorSetLayoutBindingCreateInfo
			{
				uint32_t binding;
				VkDescriptorType descriptorType;
				VkShaderStageFlags stageFlags;
				uint32_t count = 0;
				VkDescriptorBindingFlags bindingFlags = 0;
			};

			std::vector<DescriptorSetLayoutBindingCreateInfo> bindings;
		};

		DescriptorSetLayoutHandle createDescriptorSetLayout(DescriptorSetLayoutCreateInfo createInfo);

		struct DescriptorSetCreateInfo
		{
			Device& device;
			DescriptorSetLayoutHandle descriptorSetLayout;

			struct DescriptorSetWriteBufferInfo
			{
				uint32_t binding;
				VkDescriptorBufferInfo bufferInfo;
			};

			struct DescriptorSetWriteBufferArrayInfo
			{
				uint32_t binding;
				std::vector <VkDescriptorBufferInfo> bufferInfos;
			};

			struct DescriptorSetWriteImageInfo
			{
				uint32_t binding;
				VkDescriptorImageInfo imageInfo;
			};

			struct DescriptorSetWriteImageArrayInfo
			{
				uint32_t binding;
				std::vector<VkDescriptorImageInfo> imageInfos;
			};

			std::vector<DescriptorSetWriteBufferInfo> bufferWrites;
			std::vector<DescriptorSetWriteBufferArrayInfo> bufferArrayWrites;
			std::vector<DescriptorSetWriteImageInfo> imageWrites;
			std::vector<DescriptorSetWriteImageArrayInfo> imageArrayWrites;
		};

		DescriptorSetHandle createDescriptorSet(DescriptorSetCreateInfo createInfo);

		DescriptorSetLayout& getDescriptorSetLayout(DescriptorSetLayoutHandle handle);
		VkDescriptorSet getDescriptorSet(DescriptorSetHandle handle);
		std::unique_ptr<Buffer>& getUniformBuffer(UniformBufferHandle handle);


	private:
		Device& device;
		std::unique_ptr<DescriptorPool> descriptorPool;

		static std::unordered_map<UniformBufferHandle, std::unique_ptr<Buffer>> uniformBuffers;
		static std::unordered_map<DescriptorSetHandle, VkDescriptorSet> descriptorSets;
		static std::unordered_map< DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout>> descriptorSetLayouts;

		static uint32_t uniformBuffersLastHandle;
		static uint32_t descriptorSetsLastHandle;
		static uint32_t descriptorSetLayoutsLastHandle;

	};
}
