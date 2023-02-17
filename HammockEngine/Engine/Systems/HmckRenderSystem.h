#pragma once

#include "../HmckPipeline.h"
#include "../HmckDevice.h"
#include "../HmckGameObject.h"
#include "../HmckCamera.h"
#include "../HmckFrameInfo.h"
#include "../HmckDescriptors.h"
#include "../HmckSwapChain.h"
#include "../HmckModel.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

#ifndef SHADERS_DIR
	#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif



/*
 *	A system is a process which acts on all GameObjects with the desired components.
 */

/*
	At this point app functions as a Master Render System
	and this class function as subsystem

	subject to change in near future
*/

namespace Hmck
{
	class HmckRenderSystem
	{
	public:
		HmckRenderSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckRenderSystem();

		// delete copy constructor and copy destructor
		HmckRenderSystem(const HmckRenderSystem&) = delete;
		HmckRenderSystem& operator=(const HmckRenderSystem&) = delete;

		VkDescriptorSet getDescriptorSet() { return descriptorSet; }
		VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorLayout->getDescriptorSetLayout(); }

		void writeToDescriptorSet(VkDescriptorImageInfo& imageInfo);

		void renderGameObjects(HmckFrameInfo& frameInfo);


	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createPipeline(VkRenderPass renderPass);
		void prepareDescriptors();
		

		HmckDevice& hmckDevice;
		std::unique_ptr<HmckPipeline> pipeline;
		VkPipelineLayout pipelineLayout;

		// descriptors
		std::unique_ptr<HmckDescriptorPool> descriptorPool{};
		std::unique_ptr<HmckDescriptorSetLayout> descriptorLayout{};
		VkDescriptorSet descriptorSet;
		
		
	};

}
