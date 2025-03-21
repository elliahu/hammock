#include "UserInterface.h"

void UserInterface::recordUserInterface(VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = frameManager.getSwapChain()->getRenderPass();
    renderPassInfo.framebuffer = frameManager.getSwapChain()->getFramebuffer(frameManager.getSwapChainImageIndex());
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = frameManager.getSwapChain()->getSwapChainExtent();
    VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    // Begin the render pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ui.beginUserInterface();
    ui.showDemoWindow();
    ui.endUserInterface(commandBuffer);

    // End the render pass
    vkCmdEndRenderPass(commandBuffer);

}
