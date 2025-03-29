#include "VRenderPass.h"

#include <array>
#include <stdexcept>

VRenderPass::VRenderPass(VWindow& windowRef, VDevice& deviceRef): m_device{deviceRef}, m_window{windowRef} {
    recreateSwapChain();
    createCommandBuffers();
}

VRenderPass::~VRenderPass() {
    freeCommandBuffers();
}

VkCommandBuffer VRenderPass::BeginFrame() {
    assert(!m_isFrameStarted && "Frame not in progress yet??");

    auto result = m_swapChain->acquireNextImage(&m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_isFrameStarted = true;

    auto commandBuffer = GetCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    return commandBuffer;
}

void VRenderPass::endFrame() {
    assert(m_isFrameStarted && "Can't call endFrame while frame is not in progress");
    auto commandBuffer = GetCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    auto result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        m_window.wasWindowResized()) {
        m_window.resetWindowResized();
        recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

    m_isFrameStarted = false;
    m_currentFrameIndex = (m_currentFrameIndex + 1) % VSwapchain::MAX_FRAMES_IN_FLIGHT;
}

void VRenderPass::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(m_isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == GetCurrentCommandBuffer() &&
        "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain->GetRenderPass();
    renderPassInfo.framebuffer = m_swapChain->GetFrameBuffer(m_currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain->GetSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
    viewport.height = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, m_swapChain->GetSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VRenderPass::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(m_isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == GetCurrentCommandBuffer() &&
        "Can't end render pass on command buffer from a different frame");
    vkCmdEndRenderPass(commandBuffer);
}

void VRenderPass::createCommandBuffers() {
    commandBuffers.resize(VSwapchain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device.device(), &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void VRenderPass::freeCommandBuffers() {
    vkFreeCommandBuffers(
    m_device.device(),
    m_device.getCommandPool(),
    static_cast<uint32_t>(commandBuffers.size()),
    commandBuffers.data());
    commandBuffers.clear();
}

void VRenderPass::recreateSwapChain() {
    auto extent = m_window.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = m_window.getExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(m_device.device());

    if (m_swapChain == nullptr) {
        m_swapChain = std::make_unique<VSwapchain>(m_device, extent);
    } else {
        std::shared_ptr<VSwapchain> oldSwapChain = std::move(m_swapChain);
        m_swapChain = std::make_unique<VSwapchain>(m_device, extent, oldSwapChain);

        //TODO: fix cuz removed cuz broken :(

        // if (!oldSwapChain->CompareSwapFormats(*lveSwapChain.get())) {
        //     throw std::runtime_error("Swap chain image(or depth) format has changed!");
        // }
    }
}
