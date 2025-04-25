#ifndef VIMAGE_H
#define VIMAGE_H

#include "Core/Device.h"

namespace vov {
    class Image {
    public:
        explicit Image(Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        Image(Device& device, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        ~Image();

        [[nodiscard]] VkImage getImage() const { return m_image; }
        [[nodiscard]] VkImageView getImageView() const { return m_imageView; }
        [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }
        [[nodiscard]] VkDescriptorImageInfo descriptorInfo() const;

        [[nodiscard]] const std::string& getFilename() const { return m_filename; }

        void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const;

        [[nodiscard]] uint32_t getMipLevels() const { return m_mipLevels; }
        [[nodiscard]] VkSampler getSampler() const { return m_sampler; }

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

        std::string m_filename; //For checking duplicates

        VkFormat m_format{VK_FORMAT_UNDEFINED};
    };
}

#endif //VIMAGE_H
