#include "ResourceManager.h"
#include "Utils/Timer.h"

VImage* ResourceManager::loadImage(VDevice& deviceRef, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
    auto it = m_images.find(filename);
    if (it != m_images.end()) {
        std::cout << "Image already loaded: " << filename << std::endl;
        return it->second.get();
    }

    std::cout << "Image not yet loaded, Loading: " << filename << std::endl;
    auto image = std::make_unique<VImage>(deviceRef, filename, format, usage, memoryUsage);
    m_images[filename] = std::move(image);
    return m_images[filename].get();
}

void ResourceManager::clear() {
    m_images.clear();
}

ResourceManager::~ResourceManager() = default;
