#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H
#include "Core/Device.h"

namespace vov {
    class ImageView {
    public:
        ImageView(Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);
        ~ImageView();

        [[nodiscard]] VkImageView getHandle() const { return m_view; }

    private:
        Device& m_device;
        VkImageView m_view;
    };
}
#endif //IMAGEVIEW_H
