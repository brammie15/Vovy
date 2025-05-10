#include "GeoBuffer.h"

vov::GeoBuffer::GeoBuffer(vov::Device& deviceRef, VkExtent2D size): m_device{deviceRef}, m_extent{size}  {
    CreateImages();
}

void vov::GeoBuffer::Resize(VkExtent2D size) {
    DestroyImages();
    m_extent = size;
    CreateImages();
}

void vov::GeoBuffer::TransitionWriting(VkCommandBuffer commandBuffer) {
    m_RenderingAttachments.clear();
    m_RenderingAttachments.reserve(m_AllImages.size());

    for (Image*& image : m_AllImages) {
        image->TransitionImageLayout(commandBuffer,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        AddRenderAttachment(*image);
    }
}

void vov::GeoBuffer::TransitionSampling(VkCommandBuffer commandBuffer) {
    for (Image*& image : m_AllImages) {
        image->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void vov::GeoBuffer::DestroyImages() {
    m_Albedo.reset();
    m_Normal.reset();
    m_WorldPos.reset();
    m_Specularity.reset();

    m_AllImages.clear();
    m_RenderingAttachments.clear();
}

void vov::GeoBuffer::CreateImages() {
    m_Albedo = std::make_unique<Image>(
        m_device,
        m_extent.width, m_extent.height,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_Albedo->SetName("Albedo Buffer");

    m_Normal = std::make_unique<Image>(
        m_device,
        m_extent.width, m_extent.height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_Normal->SetName("Normal Buffer");

    m_WorldPos = std::make_unique<Image>(
        m_device,
        m_extent.width, m_extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_WorldPos->SetName("WorldPos Buffer");

    m_Specularity = std::make_unique<Image>(
        m_device,
        m_extent.width, m_extent.height,
        VK_FORMAT_R32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_Specularity->SetName("Specularity Buffer");

    m_AllImages.push_back(m_Albedo.get());
    m_AllImages.push_back(m_Normal.get());
    m_AllImages.push_back(m_WorldPos.get());
    m_AllImages.push_back(m_Specularity.get());
}

void vov::GeoBuffer::AddRenderAttachment(const Image& image) {
    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.imageView = image.getImageView();
    attachmentInfo.imageLayout = image.getCurrentLayout();
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue.color = { {0.f, 0.f, 0.f, 1.0f} };

    m_RenderingAttachments.emplace_back(attachmentInfo);
}
