#ifndef COMPOSITEPASS_H
#define COMPOSITEPASS_H
#include "GeometryPass.h"
#include "Core/Device.h"
#include "Rendering/Swapchain.h"
#include "Utils/FrameContext.h"

namespace vov {
    class LightingPass {
    public:

        struct PushConstant {
            glm::mat4 model;
        };

        struct alignas(16) UniformBuffer{
            glm::vec4 cameraPos;
        };

        explicit LightingPass(Device& deviceRef,uint32_t framesInFlight, VkFormat format, VkExtent2D extent);
        ~LightingPass();

        void Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, const GeometryPass& geoPass);

        void UpdateDescriptors(uint32_t frameIndex, Image& albedo, Image& normal, Image& specular, Image& bump);

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

        std::vector<std::unique_ptr<Image>> m_renderTargets{};
        VkFormat m_imageFormat{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;
    };
}


#endif //COMPOSITEPASS_H
