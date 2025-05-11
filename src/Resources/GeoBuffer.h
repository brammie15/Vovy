#ifndef GEOBUFFER_H
#define GEOBUFFER_H
#include <memory>

#include "Image.h"
#include "Core/Device.h"

namespace vov {
    class GeoBuffer final {
    public:
        explicit GeoBuffer(vov::Device& deviceRef, VkExtent2D size);
        void Resize(VkExtent2D size);

        void TransitionWriting(VkCommandBuffer commandBuffer);
        void TransitionSampling(VkCommandBuffer commandBuffer);

        [[nodiscard]] Image& GetAlbedo() const { return *m_Albedo; }
        [[nodiscard]] Image& GetNormal() const { return *m_Normal; }
        [[nodiscard]] Image& GetWorldPos() const { return *m_WorldPos; }
        [[nodiscard]] Image& GetSpecular() const { return *m_Specularity; }

        [[nodiscard]] VkExtent2D GetExtent() const { return m_extent; }

        [[nodiscard]] std::vector<VkFormat> GetFormats() const { return {m_Albedo->getFormat(), m_Normal->getFormat(), m_WorldPos->getFormat(), m_Specularity->getFormat()}; };
        [[nodiscard]] std::vector<VkRenderingAttachmentInfo>& GetRenderingAttachments() { return m_RenderingAttachments; }
        [[nodiscard]] int GetRenderAttachmentCount() const { return static_cast<int>(m_RenderingAttachments.size()); }

    private:
        Device& m_device;

        std::unique_ptr<Image> m_Albedo;
        std::unique_ptr<Image> m_Normal;
        std::unique_ptr<Image> m_WorldPos;
        std::unique_ptr<Image> m_Specularity; //Unused

        std::vector<Image*> m_AllImages{};

        VkExtent2D m_extent{};

        void DestroyImages();
        void CreateImages();

        void AddRenderAttachment(const Image& image);
        std::vector<VkRenderingAttachmentInfo> m_RenderingAttachments{};
    };
}


#endif //GEOBUFFER_H
