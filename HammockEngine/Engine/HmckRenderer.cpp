#include "HmckRenderer.h"

Hmck::HmckRenderer::HmckRenderer(HmckWindow& window, HmckDevice& device) : hmckWindow{window}, hmckDevice{device}
{
	recreateSwapChain();
	recreateOffscreenRenderPass();
	createCommandBuffer();
}

Hmck::HmckRenderer::~HmckRenderer()
{
	freeCommandBuffers();
	freeOffscreenRenderPass();
}


void Hmck::HmckRenderer::createCommandBuffer()
{
	commandBuffers.resize(HmckSwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = hmckDevice.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(hmckDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer");
	}
}


void Hmck::HmckRenderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(
		hmckDevice.device(),
		hmckDevice.getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());
	commandBuffers.clear();
}

void Hmck::HmckRenderer::freeOffscreenRenderPass()
{
	offscreenFramebuffer = nullptr;
}


void Hmck::HmckRenderer::recreateSwapChain()
{
	auto extent = hmckWindow.getExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = hmckWindow.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(hmckDevice.device());

	if (hmckSwapChain == nullptr)
	{
		hmckSwapChain = std::make_unique<HmckSwapChain>(hmckDevice, extent);
	}
	else {
		std::shared_ptr<HmckSwapChain> oldSwapChain = std::move(hmckSwapChain);
		hmckSwapChain = std::make_unique<HmckSwapChain>(hmckDevice, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*hmckSwapChain.get()))
		{
			throw std::runtime_error("Swapchain image (or detph) format has changed");
		}
	}

	//
}

VkCommandBuffer Hmck::HmckRenderer::beginFrame()
{
	assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

	auto result = hmckSwapChain->acquireNextImage(&currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to aquire swap chain image!");
	}

	isFrameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording commadn buffer");
	}
	return commandBuffer;
}

void Hmck::HmckRenderer::endFrame()
{
	assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer");
	}

	auto result = hmckSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || hmckWindow.wasWindowResized())
	{
		// Window was resized (resolution was changed)
		hmckWindow.resetWindowResizedFlag();
		recreateSwapChain();

		//freeOffscreenRenderPass();
		//recreateOffscreenRenderPass();
	}

	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false;

	currentFrameIndex = (currentFrameIndex + 1) % HmckSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Hmck::HmckRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call beginSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = hmckSwapChain->getRenderPass();
	renderPassInfo.framebuffer = hmckSwapChain->getFrameBuffer(currentImageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = hmckSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = HMCK_CLEAR_COLOR; // clear color
	clearValues[1].depthStencil = { 1.0f, 0 };
	
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(hmckSwapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(hmckSwapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0,0}, hmckSwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::HmckRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::HmckRenderer::beginOffscreenRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call beginOffscreenRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot beginOffscreenRenderPass on command buffer from a different frame");

	// TODO 

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = offscreenFramebuffer->renderPass;
	renderPassInfo.framebuffer = offscreenFramebuffer->framebuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = static_cast<uint32_t>(OFFSCREEN_RES_WIDTH);
	renderPassInfo.renderArea.extent.height = static_cast<uint32_t>(OFFSCREEN_RES_HEIGHT);

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(OFFSCREEN_RES_WIDTH);
	viewport.height = static_cast<float>(OFFSCREEN_RES_HEIGHT);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0,0}, {static_cast<float>(OFFSCREEN_RES_WIDTH), static_cast<float>(OFFSCREEN_RES_HEIGHT)} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::HmckRenderer::endOffscreenRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endOffscreenRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot endOffscreenRenderPass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::HmckRenderer::recreateOffscreenRenderPass()
{
	offscreenFramebuffer = std::make_unique<HmckFramebuffer>(hmckDevice);

	offscreenFramebuffer->width = OFFSCREEN_RES_WIDTH;
	offscreenFramebuffer->height = OFFSCREEN_RES_HEIGHT;

	// Find a suitable depth format
	VkFormat fbDepthFormat = hmckSwapChain->findDepthFormat();

	// Create sampler to sample from depth attachment
	if (offscreenFramebuffer->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create depth sampler");
	}

	// Create depth attachment
	HmckAttachmentCreateInfo attachmentInfo{};
	attachmentInfo.width = OFFSCREEN_RES_WIDTH;
	attachmentInfo.height = OFFSCREEN_RES_HEIGHT;
	attachmentInfo.layerCount = 1;
	attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	attachmentInfo.format = fbDepthFormat;
	offscreenFramebuffer->addAttachment(attachmentInfo);

	// create renderpass with framebuffer
	if (offscreenFramebuffer->createRenderPass() != VK_SUCCESS) {
		throw std::runtime_error("Failed to create offscreen renderpass");
	}
}