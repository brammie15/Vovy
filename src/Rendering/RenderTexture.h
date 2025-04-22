#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include <memory>
#include "Resources/Image.h"

namespace vov {
    class RenderTexture {
    public:
        RenderTexture(Device& device, uint32_t width, uint32_t height, VkFormat format);
        ~RenderTexture() = default;

        void beginRendering(VkCommandBuffer cmd);
        void endRendering(VkCommandBuffer cmd);

        [[nodiscard]] const Image& getTargetImage() const { return *m_colorImage; }

    private:
        Device& m_device;
        uint32_t m_width;
        uint32_t m_height;

        std::unique_ptr<Image> m_colorImage;
        std::unique_ptr<Image> m_depthImage;
    };
}

#endif
