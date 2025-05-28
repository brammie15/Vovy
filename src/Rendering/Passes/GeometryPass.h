#ifndef GEOMETRYPASS_H
#define GEOMETRYPASS_H
#include "Core/Device.h"
#include "Resources/GeoBuffer.h"
#include "Scene/Scene.h"

namespace vov {
    class GeometryPass {
    public:
        struct CreateInfo {
            int maxFrames{};
            Scene* pScene{};
            VkExtent2D size{};
            VkFormat depthFormat{};
        };

        struct alignas(16) UniformBuffer {
            glm::mat4 view;
            glm::mat4 proj;
        };

        struct PushConstant {
            glm::mat4 model;
            uint32_t objectId;
        };

        explicit GeometryPass(Device& deviceRef, const CreateInfo& createInfo);
        ~GeometryPass();

        void Record(const FrameContext& context, const Image& depthImage);

        void Resize(VkExtent2D newSize) const;

        [[nodiscard]] Image& GetAlbedo(uint32_t imageIndex) const { return m_geoBuffers[imageIndex]->GetAlbedo(); }
        [[nodiscard]] Image& GetNormal(uint32_t imageIndex) const { return m_geoBuffers[imageIndex]->GetNormal(); }
        [[nodiscard]] Image& GetSpecualar(uint32_t imageIndex) const { return m_geoBuffers[imageIndex]->GetSpecular(); }
        [[nodiscard]] Image& GetWorldPos(uint32_t imageIndex) const { return m_geoBuffers[imageIndex]->GetWorldPos(); }

        using PixelCallback = std::function<void(glm::vec3)>;

    private:
        Device& m_device;

        VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};
        uint32_t m_framesInFlight{1};

        std::unique_ptr<DescriptorPool> m_descriptorPool{};

        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

        std::unique_ptr<Pipeline> m_pipeline{};

        std::vector<std::unique_ptr<Buffer>> m_uniformBuffers{};
        std::vector<VkDescriptorSet> m_descriptorSets{};

        std::unique_ptr<DescriptorSetLayout> m_textureSetLayout{};
        VkDescriptorSet m_textureSet{};

        std::vector<std::unique_ptr<GeoBuffer>> m_geoBuffers{};

    };
}


#endif //GEOMETRYPASS_H
