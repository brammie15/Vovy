#include "ResourceManager.h"

#include <filesystem>
#include <iostream>

#include "Utils/Timer.h"

namespace vov {
    Image *ResourceManager::LoadImage(Device& deviceRef, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
        const auto it = m_images.find(filename);
        if (it != m_images.end()) {
            if (!filename.empty() && (std::filesystem::exists(filename) && std::filesystem::is_regular_file(filename))) {
                std::cout << "Image already loaded: " << filename << std::endl;
            }
            return it->second.get();
        }

        std::cout << "Image not yet loaded, Loading: " << filename << std::endl;
        auto image = std::make_unique<Image>(deviceRef, filename, format, usage, memoryUsage);
        m_images[filename] = std::move(image);
        return m_images[filename].get();
    }

    void ResourceManager::Clear() {
        m_images.clear();
        m_dummyImage.reset();
        std::cout << "ResourceManager cleared." << std::endl;
    }

    //TODO: kinda bad since when 2 meshes uses the same texture i need to check
    void ResourceManager::UnloadImage(Image* image) {
        if (image != nullptr) {
            m_images.erase(image->getFilename());
        }
    }

    Image* ResourceManager::LoadDummyImage(Device& deviceRef) {
        if (m_dummyImage != nullptr) {
            return m_dummyImage.get();
        }

        constexpr uint8_t pixel[] = {255, 0, 255, 255}; // Magenta RGBA

        VkExtent2D extent = {1, 1};
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        constexpr VkDeviceSize imageSize = sizeof(pixel);

        const Buffer stagingBuffer(deviceRef, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        stagingBuffer.copyTo(pixel, imageSize);

        m_dummyImage = std::make_unique<Image>(
            deviceRef,
            extent,
            format,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            true,
            true
        );

        deviceRef.TransitionImageLayout(m_dummyImage->getImage(), format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        deviceRef.copyBufferToImage(stagingBuffer.getBuffer(), m_dummyImage->getImage(), extent.width, extent.height);
        deviceRef.TransitionImageLayout(m_dummyImage->getImage(), format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        m_dummyImage->SetName("Dummy 1x1");

        return m_dummyImage.get();
    }

    ResourceManager::~ResourceManager() = default;
}
