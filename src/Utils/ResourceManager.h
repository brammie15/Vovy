#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <memory>
#include <unordered_map>

#include "Singleton.h"
#include "Resources/Image.h"
namespace vov {
    class ResourceManager final: public Singleton<ResourceManager> {
    public:

        Image* loadImage(Device& deviceRef, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        void clear();

    private:

        std::unordered_map<std::string, std::unique_ptr<Image>> m_images;

        ResourceManager() = default;

        ~ResourceManager() override;
        friend class Singleton<ResourceManager>;
    };
}


#endif //RESOURCEMANAGER_H
