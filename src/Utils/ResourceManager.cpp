#include "ResourceManager.h"

VImage* ResourceManager::loadImage(VDevice& deviceRef, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
    auto it = m_images.find(filename);
    if (it != m_images.end()) {
        std::cout << "Image already loaded: " << filename << std::endl;
        return it->second.get();
    }

    std::cout << "Image not yet loaded, Loading: " << filename << std::endl;
    auto image = std::make_unique<VImage>(deviceRef, filename, format, usage, memoryUsage);
    VImage* imagePtr = image.get();
    m_images[filename] = std::move(image);
    return imagePtr;
}

void ResourceManager::clear() {
    m_images.clear();
}

ResourceManager::~ResourceManager() {
}
