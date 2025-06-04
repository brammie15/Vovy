#ifndef SHADOWPASS_H
#define SHADOWPASS_H

#include "Core/Device.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Rendering/Pipeline.h"
#include "Resources/Image.h"
#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Utils/FrameContext.h"

namespace vov {
    class ShadowPass {
    public:
        struct UniformBuffer {
            glm::mat4 lightViewMatrix{};
            glm::mat4 lightProjectionMatrix{};
        };

        struct PushConstant {
            glm::mat4 model;
            uint32_t objectId{0};
        };

        explicit ShadowPass(Device& deviceRef, uint32_t framesInFlight, VkFormat format, VkExtent2D extent, DirectionalLight& directionalLight);
        ~ShadowPass();

        void Record(const FrameContext& context);

        void Resize(VkExtent2D newSize);
        Image& GetDepthImage(int frameIndex);

    private:
        Device& m_device;
        uint32_t m_framesInFlight{};
        VkFormat m_imageFormat{};

        DirectionalLight& m_directionalLight;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};
        std::vector<VkDescriptorSet> m_descriptorSets{};
        std::vector<std::unique_ptr<Buffer>> m_uniformBuffers{};

        //TODO: make this a vector of unique_ptrs
        std::unique_ptr<Image> m_depthImage{}; //The image i render depth for directionallight to

        std::unique_ptr<Pipeline> m_pipeline{};
        VkPipelineLayout m_pipelineLayout{};
    };
}


#endif //SHADOWPASS_H
