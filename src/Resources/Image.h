#ifndef VIMAGE_H
#define VIMAGE_H

#include <memory>

#include "Core/Device.h"
#include "Image/ImageView.h"
#include "Image/Sampler.h"

namespace vov {
    class ImageView;

    class Image {
    public:
        explicit Image(
            Device& device,
            VkExtent2D size,
            VkFormat format,
            VkImageUsageFlags usage,
            VmaMemoryUsage memoryUsage,
            bool createView = true,
            bool createSampler = true,
            VkFilter filter = VK_FILTER_LINEAR
        );
        Image(Device& device, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkFilter filter = VK_FILTER_LINEAR);

        //Used for swapchain only
        Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage existingImage);
        ~Image();

        [[nodiscard]] VkImage getImage() const { return m_image; }
        [[nodiscard]] VkImageView GetImageView() const { return m_imageView->getHandle(); }
        [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }
        [[nodiscard]] VkDescriptorImageInfo descriptorInfo() const;

		[[nodiscard]] VkImageLayout GetCurrentLayout() const { return m_imageLayout; }

        [[nodiscard]] const std::string& getFilename() const { return m_filename; }

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);
        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

        void SetName(const std::string& name);

        [[nodiscard]] uint32_t getMipLevels() const { return m_mipLevels; }
        [[nodiscard]] VkSampler getSampler() const { return m_sampler->getHandle(); }
        [[nodiscard]] VkExtent2D GetExtent() const { return m_extent; }
        [[nodiscard]] VkFormat GetFormat() const { return m_format; }

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
        void createImage(VkExtent2D size, uint32_t miplevels, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        void createImageView(VkFormat format);
        void createImageSampler(VkFilter filter, VkSamplerAddressMode addressMode);
        void generateMipmaps(VkFormat format, uint32_t width, uint32_t height) const;

        static VkImageAspectFlags getImageAspect(VkFormat format);

        Device& m_device;
        VkImage m_image;
        VmaAllocation m_allocation;

        std::unique_ptr<ImageView> m_imageView;
        std::unique_ptr<Sampler> m_sampler;

        uint32_t m_mipLevels{};

        VkExtent2D m_extent{};

        std::string m_filename; //For checking duplicates

        VkImageLayout m_imageLayout{VK_IMAGE_LAYOUT_UNDEFINED};

        VkFormat m_format{VK_FORMAT_UNDEFINED};

        bool m_isSwapchainImage{ false }; // Indicates if this image is part of the swapchain
    };
}

#endif //VIMAGE_H
