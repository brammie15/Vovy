#include "RenderTexture.h"
#include <array>
#include <stdexcept>
#include <vulkan/vulkan.h>

namespace vov {

    RenderTexture::RenderTexture(Device& device, uint32_t width, uint32_t height, VkFormat format, bool isDepthOnly)
        : m_device(device), m_width(width), m_height(height), m_isDepthOnly(isDepthOnly) {
        VkExtent2D extent = {width, height};

        if (!m_isDepthOnly) {
            m_colorImage = std::make_unique<Image>(
                device, extent, format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            );
        }

        // Create depth image only if needed or if this is a depth-only target
        VkFormat depthFormat = isDepthOnly ? format : VK_FORMAT_D32_SFLOAT;
        m_depthImage = std::make_unique<Image>(
            device,extent, depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | (isDepthOnly ? VK_IMAGE_USAGE_SAMPLED_BIT : 0),
            VMA_MEMORY_USAGE_GPU_ONLY
        );
    }

    void RenderTexture::beginRendering(VkCommandBuffer cmd) {
        std::array<VkClearValue, 2> clearValues{};
        if (!m_isDepthOnly) {
            clearValues[0].color = {{1.0f, 0.0f, 0.0f, 1.0f}};
        }
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        if (!m_isDepthOnly) {
            colorAttachmentInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_colorImage->GetImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = clearValues[0]
            };
        }

        VkRenderingAttachmentInfo depthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_depthImage->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = m_isDepthOnly ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = clearValues[1]
        };

        const VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, {m_width, m_height}},
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(m_isDepthOnly ? 0 : 1),
            .pColorAttachments = m_isDepthOnly ? nullptr : &colorAttachmentInfo,
            .pDepthAttachment = &depthAttachmentInfo
        };

        vkCmdBeginRendering(cmd, &renderingInfo);

        const VkViewport viewport{0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
        const VkRect2D scissor{{0, 0}, {m_width, m_height}};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void RenderTexture::endRendering(VkCommandBuffer cmd) {
        vkCmdEndRendering(cmd);
    }
}