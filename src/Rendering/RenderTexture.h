#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include <memory>

#include "Resources/Image.h"

namespace vov {
    class RenderTexture {
    public:
        RenderTexture(Device& device, uint32_t width, uint32_t height, VkFormat format);
        ~RenderTexture();

        void beginRenderPass(VkCommandBuffer cmd);
        void endRenderPass(VkCommandBuffer cmd);

        [[nodiscard]] const Image& getTargetImage() const { return *m_colorImage; }
        [[nodiscard]] VkRenderPass getRenderPass() const { return m_renderPass; }
        [[nodiscard]] VkFramebuffer getFramebuffer() const { return m_framebuffer; }

    private:
        void createRenderPass(VkFormat format);
        void createFramebuffer();

        Device& m_device;
        uint32_t m_width;
        uint32_t m_height;

        std::unique_ptr<Image> m_colorImage;
        std::unique_ptr<Image> m_depthImage;
        VkRenderPass m_renderPass{};
        VkFramebuffer m_framebuffer{};
    };
}

#endif