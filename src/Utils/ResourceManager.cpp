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
    }

    //TODO: kinda bad since when 2 meshes uses the same texture i need to check
    void ResourceManager::UnloadImage(Image* image) {
        if (image != nullptr) {
            m_images.erase(image->getFilename());
        }
    }

    ResourceManager::~ResourceManager() = default;
}
