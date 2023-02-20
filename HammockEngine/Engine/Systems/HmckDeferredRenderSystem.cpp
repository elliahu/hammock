#include "HmckDeferredRenderSystem.h"

Hmck::HmckDeferredRenderSystem::HmckDeferredRenderSystem(
	HmckDevice& device, VkRenderPass renderPass, 
	std::vector<VkDescriptorSetLayout>& setLayouts): HmckIRenderSystem(device)
{
	prepareDescriptors();
	createPipelineLayout(setLayouts);
	createPipeline(renderPass);
}

Hmck::HmckDeferredRenderSystem::~HmckDeferredRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckDeferredRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	std::vector<VkDescriptorSetLayout> layouts{ setLayouts };
	layouts.push_back(gbufferDescriptorLayout->getDescriptorSetLayout());
	layouts.push_back(shadowmapDescriptorLayout->getDescriptorSetLayout());


	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckDeferredRenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/deferred.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/deferred.frag.spv",
		pipelineConfig
	);
}


void Hmck::HmckDeferredRenderSystem::render(HmckFrameInfo& frameInfo)
{
	pipeline->bind(frameInfo.commandBuffer);

	// bind descriptor set (UBO)
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	// gbuffer
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		1, 1,
		&gbufferDescriptorSet,
		0,
		nullptr
	);

	// shadowmap
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		2, 1,
		&shadowmapDescriptorSet,
		0,
		nullptr
	);
	vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);

	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;
		if (obj.meshComponent == nullptr) continue;

		
	}

}

void Hmck::HmckDeferredRenderSystem::prepareDescriptors()
{
	descriptorPool = HmckDescriptorPool::Builder(hmckDevice)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8)
		.build();

	shadowmapDescriptorLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	gbufferDescriptorLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();
}

void Hmck::HmckDeferredRenderSystem::updateShadowmapDescriptorSet(VkDescriptorImageInfo& imageInfo)
{
	auto writer = HmckDescriptorWriter(*shadowmapDescriptorLayout, *descriptorPool)
		.writeImage(0, &imageInfo)
		.build(shadowmapDescriptorSet);
}

void Hmck::HmckDeferredRenderSystem::updateGbufferDescriptorSet(std::array<VkDescriptorImageInfo, 7> imageInfos)
{
	auto writer = HmckDescriptorWriter(*gbufferDescriptorLayout, *descriptorPool)
		.writeImage(0, &imageInfos[0])
		.writeImage(1, &imageInfos[1])
		.writeImage(2, &imageInfos[2])
		.writeImage(3, &imageInfos[3])
		.writeImage(4, &imageInfos[4])
		.writeImage(5, &imageInfos[5])
		.writeImage(6, &imageInfos[6])
		.build(gbufferDescriptorSet);
}