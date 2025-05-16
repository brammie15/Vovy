#ifndef VIMAGE_H
#define VIMAGE_H

#include "Core/Device.h"

namespace vov {
    class Image {
    public:
        explicit Image(Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        Image(Device& device, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);

        //Used for swapchain only
        Image(Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage existingImage);
        ~Image();

        [[nodiscard]] VkImage getImage() const { return m_image; }
        [[nodiscard]] VkImageView getImageView() const { return m_imageView; }
        [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }
        [[nodiscard]] VkDescriptorImageInfo descriptorInfo() const;

		[[nodiscard]] VkImageLayout getCurrentLayout() const { return m_imageLayout; }

        [[nodiscard]] const std::string& getFilename() const { return m_filename; }

        // void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const;

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

        void SetName(const std::string& name);

        [[nodiscard]] uint32_t getMipLevels() const { return m_mipLevels; }
        [[nodiscard]] VkSampler getSampler() const { return m_sampler; }
        VkExtent2D getExtent() const {
            return m_extent;
        }

        [[nodiscard]] VkFormat getFormat() const { return m_format; }

        [[nodiscard]] bool HasStencil() const {
            switch (m_format)
            {
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] bool HasDepth() const {
            switch (m_format)
            {
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    return true;
                default:
                    return false;
            }
        }

    private:
        void createImage(uint32_t width, uint32_t height, uint32_t miplevels, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        void createImageView(VkFormat format);
        void createSampler(VkFilter filter, VkSamplerAddressMode addressMode);
        void generateMipmaps(VkFormat format, uint32_t width, uint32_t height) const;

        static VkImageAspectFlags getImageAspect(VkFormat format);

        Device& m_device;
        VkImage m_image;
        VmaAllocation m_allocation;
        VkImageView m_imageView;
        VkSampler m_sampler{};

        uint32_t m_mipLevels{};

        VkExtent2D m_extent{};

        std::string m_filename; //For checking duplicates

        VkImageLayout m_imageLayout{VK_IMAGE_LAYOUT_UNDEFINED};

        VkFormat m_format{VK_FORMAT_UNDEFINED};

        bool m_isSwapchainImage{ false }; // Indicates if this image is part of the swapchain
    };
}

#endif //VIMAGE_H
