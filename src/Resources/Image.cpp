#include "Image.h"

#include "Buffer.h"


#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <stdexcept>

#include "Image/ImageView.h"
#include "Utils/DebugLabel.h"
#include "Utils/stb_image.h"

// #include <gli/gli.hpp>

namespace vov {
    Image::Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, bool createView, bool createSampler, VkFilter filter)
    : m_device(device), m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE),
      m_imageView(VK_NULL_HANDLE), m_mipLevels(1), m_format{format} {  // Initialize m_mipLevels here
        createImage(size, 1, format, usage, memoryUsage);
        if (createView) {
            createImageView(format);
        }
        if (createSampler) {
            createImageSampler(filter, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        }
        m_extent = size;
    }

    Image::Image(Device& device, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkFilter filter)
        : m_device(device), m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_imageView(VK_NULL_HANDLE), m_filename{filename}, m_format{format} {

        uint8_t* pixels = nullptr;
        VkDeviceSize imageSize{};
        int texWidth, texHeight, texChannels;
        bool usingStb = true;
        // gli::texture texture;

        const std::string fileExtension = filename.substr(filename.find_last_of('.') + 1);
        if (fileExtension == "dds") {
            // texture = gli::load(filename);
            // if (texture.empty()) {
            //     std::cerr << "Failed to load DDS texture image!" << std::endl;
            //     pixels = stbi_load("resources/TextureNotFound.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            // } else {
            //     texWidth = static_cast<int>(texture.extent().x);
            //     texHeight = static_cast<int>(texture.extent().y);
            //
            //     pixels = static_cast<uint8_t*>(texture.data());
            //     format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            //     imageSize = static_cast<VkDeviceSize>(texture.size());
            //     m_mipLevels = static_cast<uint32_t>(texture.levels());
            //     m_extent = VkExtent2D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)};
            //     usingStb = false;
            // }
        } else {
            pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels) {
                std::cerr << "Failed to load texture image!" << std::endl;
                pixels = stbi_load("resources/TextureNotFound.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            }
            // m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
            m_mipLevels = 1;
            m_extent = VkExtent2D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)};
            imageSize = texWidth * texHeight * 4;
        }

        // Create a staging buffer
        const Buffer stagingBuffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        //TODO: should prob actually use the VMA auto mapper, o well :p
        stagingBuffer.copyTo(pixels, imageSize);

        if (usingStb) {
            stbi_image_free(pixels);
        }

        createImage(m_extent, m_mipLevels, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, memoryUsage);
        createImageView(format);

        device.TransitionImageLayout(m_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
        device.copyBufferToImage(stagingBuffer.getBuffer(), m_image, texWidth, texHeight);
        if(usingStb)  {
            generateMipmaps(format, texWidth, texHeight);
        }
        // device.TransitionImageLayout(m_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);

        createImageSampler(filter, VK_SAMPLER_ADDRESS_MODE_REPEAT);

        DebugLabel::NameImage(m_image, filename);
    }

    Image::Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage existingImage)
    : m_device(device), m_image(existingImage), m_allocation(VK_NULL_HANDLE),
      m_imageView(VK_NULL_HANDLE), m_mipLevels(1), m_extent{size}, m_format(format) {
        createImageView(format);
        m_isSwapchainImage = true; // Mark this image as a swapchain image
    }

    Image::~Image() {
        if (!m_isSwapchainImage) {
            vmaDestroyImage(m_device.allocator(), m_image, m_allocation);
        }
    }

    VkDescriptorImageInfo Image::descriptorInfo() const {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = m_sampler->getHandle();
        imageInfo.imageView = m_imageView->getHandle();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        return imageInfo;
    }


    void Image::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_imageLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;

        barrier.subresourceRange.aspectMask = 0;
        if (HasDepth())
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (HasStencil()) barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        m_imageLayout = newLayout;
    }

    void Image::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout,
                                 VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_imageLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;

        barrier.subresourceRange.aspectMask = 0;
        if (HasDepth()) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (HasStencil()) barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStageMask,  // Use provided source stage mask
            dstStageMask,  // Use provided destination stage mask
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        m_imageLayout = newLayout;
    }

    void Image::SetName(const std::string& name) {
        DebugLabel::SetObjectName(reinterpret_cast<uint64_t>(m_image), VK_OBJECT_TYPE_IMAGE, name.c_str());
    }

    void Image::createImage(VkExtent2D size, uint32_t miplevels, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.height = size.height;
        imageInfo.extent.width = size.width;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = miplevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;

        if (vmaCreateImage(m_device.allocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image with VMA!");
        }
    }

    void Image::createImageView(VkFormat format) {
        m_imageView = std::make_unique<ImageView>(m_device, m_image, format, getImageAspect(format), m_mipLevels);
    }

    void Image::createImageSampler(const VkFilter filter, const VkSamplerAddressMode addressMode) {
        m_sampler = std::make_unique<Sampler>(m_device, filter, addressMode, m_mipLevels);
    }

    void Image::generateMipmaps(VkFormat format, uint32_t width, uint32_t height) const {
        const VkFormatProperties properties = m_device.GetFormatProperties(format);
        VkImageAspectFlags aspect = getImageAspect(format);

        if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = m_device.beginSingleTimeCommands();

        auto mipWidth = static_cast<int32_t>(width);
        auto mipHeight = static_cast<int32_t>(height);

        for (uint32_t i = 1; i < m_mipLevels; i++) {
            // Transition previous mip level to SRC_OPTIMAL
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = m_image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = aspect;
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);

            // Transition next mip level to DST_OPTIMAL
            barrier.subresourceRange.baseMipLevel = i;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);

            // Blit
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;

            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                mipWidth > 1 ? mipWidth / 2 : 1,
                mipHeight > 1 ? mipHeight / 2 : 1,
                1
            };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            // Transition previous level to SHADER_READ_ONLY_OPTIMAL
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);

            mipWidth = std::max(mipWidth / 2, 1);
            mipHeight = std::max(mipHeight / 2, 1);
        }
        //
        // // Transition last mip level to SHADER_READ_ONLY_OPTIMAL
        VkImageMemoryBarrier lastBarrier{};
        lastBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        lastBarrier.image = m_image;
        lastBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lastBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lastBarrier.subresourceRange.aspectMask = aspect;
        lastBarrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
        lastBarrier.subresourceRange.levelCount = 1;
        lastBarrier.subresourceRange.baseArrayLayer = 0;
        lastBarrier.subresourceRange.layerCount = 1;

        lastBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        lastBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        lastBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        lastBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &lastBarrier);

        // m_device.TransitionImageLayout(m_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);
        m_device.endSingleTimeCommands(commandBuffer);
    }

    //Thanks ChatGPT
    VkImageAspectFlags Image::getImageAspect(VkFormat format) {
        switch (format) {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
}
