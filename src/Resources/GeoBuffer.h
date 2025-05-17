#ifndef GEOBUFFER_H
#define GEOBUFFER_H
#include <memory>

#include "Image.h"
#include "Core/Device.h"
#include "glm/vec3.hpp"

namespace vov {
    class GeoBuffer final {
    public:
        explicit GeoBuffer(vov::Device& deviceRef, VkExtent2D size);
        void Resize(VkExtent2D size);

        void TransitionWriting(VkCommandBuffer commandBuffer);
        void TransitionSampling(VkCommandBuffer commandBuffer);

        [[nodiscard]] Image& GetAlbedo() const { return *m_albedo; }
        [[nodiscard]] Image& GetNormal() const { return *m_normal; }
        [[nodiscard]] Image& GetWorldPos() const { return *m_worldPos; }
        [[nodiscard]] Image& GetSpecular() const { return *m_metalicRoughness; }
        [[nodiscard]] Image& GetSelection() const { return *m_selectionImage; }

        [[nodiscard]] VkExtent2D GetExtent() const { return m_extent; }

        [[nodiscard]] std::vector<VkFormat> GetFormats() const { return {m_albedo->getFormat(), m_normal->getFormat(), m_worldPos->getFormat(), m_metalicRoughness->getFormat(), m_selectionImage->getFormat()}; }
        [[nodiscard]] std::vector<VkRenderingAttachmentInfo>& GetRenderingAttachments() { return m_RenderingAttachments; }
        [[nodiscard]] int GetRenderAttachmentCount() const { return static_cast<int>(m_RenderingAttachments.size()); }

        glm::vec3 GetPixel(struct ::VkCommandBuffer_T* commandBuffer, glm::ivec2 screenPos, glm::ivec2 viewportSize);

        void TransitionSelectionImageToTransferSrc(VkCommandBuffer commandBuffer) {;
            m_selectionImage->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        }

        void TransitionSelectionImageToColorAttachment(VkCommandBuffer commandBuffer) {
            m_selectionImage->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

    private:
        Device& m_device;

        std::unique_ptr<Image> m_albedo;
        std::unique_ptr<Image> m_normal;
        std::unique_ptr<Image> m_worldPos;
        std::unique_ptr<Image> m_metalicRoughness;
        std::unique_ptr<Image> m_selectionImage;

        std::vector<Image*> m_allImages{};

        VkExtent2D m_extent{};

        void DestroyImages();
        void CreateImages();

        void AddRenderAttachment(const Image& image);
        std::vector<VkRenderingAttachmentInfo> m_RenderingAttachments{};
    };
}


#endif //GEOBUFFER_H
