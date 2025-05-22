#include "Renderer.h"

#include <array>
#include <stdexcept>
#include <ranges>

#include "Utils/DebugLabel.h"


namespace vov {
    Renderer::Renderer(Window& windowRef, Device& deviceRef): m_window{windowRef}, m_device{deviceRef} {
        recreateSwapChain();
        createCommandBuffers();
    }

    Renderer::~Renderer() {
        freeCommandBuffers();
    }

    VkCommandBuffer Renderer::BeginFrame() {
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

    void Renderer::endFrame() {
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
        m_currentFrameIndex = (m_currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) const {
        assert(m_isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
        assert(
            commandBuffer == GetCurrentCommandBuffer() &&
            "Can't begin render pass on command buffer from a different frame");

        m_swapChain->GetImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
            commandBuffer,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
            commandBuffer,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        const VkRenderingAttachmentInfoKHR color_attachment_info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_swapChain->GetImageView(static_cast<int>(m_currentImageIndex)),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clearValues[0],
        };

        const VkRenderingAttachmentInfo depth_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex)).getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            // .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clearValues[1],
        };

        const VkRenderingInfoKHR render_info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = {
                .offset = {0, 0},
                .extent = m_swapChain->GetSwapChainExtent(),
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_info,
            .pDepthAttachment = &depth_attachment_info,
        };

        DebugLabel::BeginCmdLabel(
            commandBuffer,
            "RenderPass",
            {1.f, 1.0f, 0.0f, 1.0f}
        );

        vov::vkCmdBeginRenderingKHR(commandBuffer, &render_info);

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

    void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) const {
        assert(m_isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
        assert(
            commandBuffer == GetCurrentCommandBuffer() &&
            "Can't end render pass on command buffer from a different frame");
        // vkCmdEndRenderPass(commandBuffer);
        vov::vkCmdEndRenderingKHR(commandBuffer);

        m_swapChain->GetImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
            commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

        m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
            commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        DebugLabel::EndCmdLabel(commandBuffer);
    }

    void Renderer::createCommandBuffers() {
        commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_device.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(m_device.device(), &allocInfo, commandBuffers.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (int index = 0; index < commandBuffers.size(); ++index) {
            DebugLabel::NameCommandBuffer(commandBuffers[index], "CommandBuffer: " + std::to_string(index));
        }
    }

    void Renderer::freeCommandBuffers() {
        vkFreeCommandBuffers(
            m_device.device(),
            m_device.getCommandPool(),
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
        commandBuffers.clear();
    }

    void Renderer::recreateSwapChain() {
        auto extent = m_window.getExtent();
        while (extent.width == 0 || extent.height == 0) {
            extent = m_window.getExtent();
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(m_device.device());

        if (m_resizeCallback != nullptr) {
            m_resizeCallback(extent);
        }

        if (m_swapChain == nullptr) {
            m_swapChain = std::make_unique<Swapchain>(m_device, extent);
        } else {
            std::shared_ptr<Swapchain> oldSwapChain = std::move(m_swapChain);
            m_swapChain = std::make_unique<Swapchain>(m_device, extent, oldSwapChain);

            //TODO: fix cuz removed cuz broken :(

            // if (!oldSwapChain->CompareSwapFormats(*lveSwapChain.get())) {
            //     throw std::runtime_error("Swap chain image(or depth) format has changed!");
            // }
        }
    }
}
