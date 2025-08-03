#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <memory>
#include <unordered_map>

#include "Singleton.h"
#include "Resources/Buffer.h"
#include "Resources/Image.h"
namespace vov {
    class ResourceManager final: public Singleton<ResourceManager> {
    public:

        Image* LoadImage(Device& deviceRef, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        void Clear();
        void UnloadImage(Image* image);

        Image* LoadDummyImage(Device& deviceRef);

    private:

        std::unordered_map<std::string, std::unique_ptr<Image>> m_images;

        std::unique_ptr<Image> m_dummyImage;

        ResourceManager() = default;

        ~ResourceManager() override;
        friend class Singleton<ResourceManager>;
    };
}


#endif //RESOURCEMANAGER_H
