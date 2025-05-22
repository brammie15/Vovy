#include "ImageView.h"

#include <stdexcept>

vov::ImageView::ImageView(Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
    : m_device(device) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &m_view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view");
    }
}

vov::ImageView::~ImageView() {
    if (m_view) {
        vkDestroyImageView(m_device.device(), m_view, nullptr);
    }
}