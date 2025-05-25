#include "GeoBuffer.h"

#include "Buffer.h"
#include "glm/vec4.hpp"
#include "Rendering/Passes/BlitPass.h"
#include "Rendering/Passes/BlitPass.h"
#include "Rendering/Passes/BlitPass.h"
#include "Rendering/Passes/BlitPass.h"
#include "Utils/BezierCurves.h"
#include "Utils/BezierCurves.h"

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
    m_RenderingAttachments.reserve(m_allImages.size());

    for (Image*& image : m_allImages) {
        image->TransitionImageLayout(commandBuffer,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        AddRenderAttachment(*image);
    }
}

void vov::GeoBuffer::TransitionSampling(VkCommandBuffer commandBuffer) {
    for (Image*& image : m_allImages) {
        image->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

glm::vec3 vov::GeoBuffer::GetPixel(VkCommandBuffer commandBuffer,glm::ivec2 screenPos, glm::ivec2 viewportSize) {
    int32_t flippedY{ static_cast<int32_t>(viewportSize.y - screenPos.y) };
    glm::vec2 pixelPos{ static_cast<float>(screenPos.x) / static_cast<float>(viewportSize.x), static_cast<float>(flippedY) / static_cast<float>(viewportSize.y) };

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { screenPos.x, flippedY, 0 };
    region.imageExtent = { 1, 1, 1 };

    Buffer stagingBuffer{
        m_device,
        sizeof(glm::vec4),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_TO_CPU
    };

    vkCmdCopyImageToBuffer(commandBuffer, m_selectionImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer.getBuffer(), 1, &region);

    stagingBuffer.map();
    uint8_t* color = reinterpret_cast<uint8_t*>(stagingBuffer.GetRawData()); // Assuming VK_FORMAT_R8G8B8A8_UNORM
    glm::vec4 pixelData{};

    pixelData = glm::vec4(color[0], color[1], color[2], color[3]);
    stagingBuffer.unmap();

    return glm::vec3(pixelData.r, pixelData.g, pixelData.b);
}

void vov::GeoBuffer::DestroyImages() {
    m_albedo.reset();
    m_normal.reset();
    m_worldPos.reset();
    m_metalicRoughness.reset();

    m_allImages.clear();
    m_RenderingAttachments.clear();
}

void vov::GeoBuffer::CreateImages() {
    m_albedo = std::make_unique<Image>(
        m_device,
        m_extent,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_albedo->SetName("Albedo Buffer");

    m_normal = std::make_unique<Image>(
        m_device,
        m_extent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_normal->SetName("Normal Buffer");

    m_worldPos = std::make_unique<Image>(
        m_device,
        m_extent,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_worldPos->SetName("WorldPos Buffer");

    m_metalicRoughness = std::make_unique<Image>(
        m_device,
        m_extent,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_metalicRoughness->SetName("MetallicRoughness Buffer");

    m_selectionImage = std::make_unique<Image>(
        m_device,
        m_extent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO
    );
    m_selectionImage->SetName("Selection Buffer");

    m_allImages.push_back(m_albedo.get());
    m_allImages.push_back(m_normal.get());
    m_allImages.push_back(m_worldPos.get());
    m_allImages.push_back(m_metalicRoughness.get());
    m_allImages.push_back(m_selectionImage.get());
}

void vov::GeoBuffer::AddRenderAttachment(const Image& image) {
    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.imageView = image.GetImageView();
    attachmentInfo.imageLayout = image.GetCurrentLayout();
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue.color = { {0.f, 0.f, 0.f, 1.0f} };

    m_RenderingAttachments.emplace_back(attachmentInfo);
}
