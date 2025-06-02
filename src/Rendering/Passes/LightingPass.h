#ifndef COMPOSITEPASS_H
#define COMPOSITEPASS_H
#include "GeometryPass.h"
#include "ShadowPass.h"
#include "Core/Device.h"
#include "Rendering/Swapchain.h"
#include "Resources/HDRI.h"
#include "Utils/FrameContext.h"

namespace vov {
    class LightingPass {
    public:

        struct PushConstant {
            glm::mat4 model;
        };

        struct alignas(16) DirectionalLightInfo {
            glm::vec3 direction{};
            float _pad0;           // padding to align next vec3

            glm::vec3 color{};
            float intensity{};

            glm::mat4 lightProjView{};
        };

        struct alignas(16) UniformBuffer{
            glm::mat4 proj{};
            glm::mat4 view{};
            Camera::CameraSettings camSettings{};
            DirectionalLightInfo lightInfo{};
            uint32_t  pointLightCount{};
            float _pad2[3];
            glm::vec2 viewportSize{};
            glm::vec2 _pad1{};
            int debugViewMode{static_cast<int>(DebugView::NONE)};
        };

        explicit LightingPass(Device& deviceRef,uint32_t framesInFlight, VkFormat format, VkExtent2D extent, const HDRI* hdri = nullptr);
        ~LightingPass();

        void Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, const GeometryPass& geoPass, const HDRI& hdri, ShadowPass& shadowPass, Scene& scene);

        void UpdateDescriptors(uint32_t frameIndex, const Image& albedo, const Image& normal, const Image& specular, const Image& bump, const Image& depth, const Image& shadowMap);

        [[nodiscard]] Image& GetImage(int imageIndex) const;
        void Resize(VkExtent2D newSize);

    private:

        Device& m_device;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        uint32_t m_framesInFlight{};

        std::unique_ptr<DescriptorSetLayout> m_geobufferSamplersSetLayout{};
        std::vector<VkDescriptorSet> m_textureDescriptors{};

        std::vector<std::unique_ptr<Buffer>> m_uniformBuffers{};
        std::vector<VkDescriptorSet> m_descriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};

        std::vector<std::unique_ptr<Buffer>> m_pointLightBuffers{};
        std::vector<VkDescriptorSet> m_pointLightDescriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_pointLightSetLayout{};

        std::unique_ptr<DescriptorSetLayout> m_hdriSamplerSetLayout{};
        VkDescriptorSet m_hdriSamplerDescriptorSets{};

        std::vector<std::unique_ptr<Image>> m_renderTargets{};
        VkFormat m_imageFormat{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;
    };
}


#endif //COMPOSITEPASS_H
