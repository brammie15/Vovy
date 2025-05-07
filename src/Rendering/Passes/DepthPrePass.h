#ifndef DEPTHPREPASS_H
#define DEPTHPREPASS_H
#include "Core/Device.h"
#include "Descriptors/DescriptorPool.h"

#include <glm/glm.hpp>

#include "Resources/Image.h"
#include "Scene/Scene.h"
#include "Utils/FrameContext.h"

namespace vov {
    class Pipeline;
    class DescriptorSetLayout;

    class DepthPrePass final {
    public:

        explicit DepthPrePass(Device& deviceRef) : m_device{deviceRef} {}
        ~DepthPrePass();

        void Init(VkFormat depthFormat, uint32_t framesInFlight);
        void Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, Image& depthImage, Scene* scene, Camera* camera);

        struct alignas(16) UniformBuffer
        {
            glm::mat4 view;
            glm::mat4 proj;
        };
        struct PushConstant
        {
            glm::mat4 model;
        };

    private:


        Device& m_device;
        VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};
        uint32_t m_framesInFlight{1};

        std::unique_ptr<DescriptorPool> m_descriptorPool;

        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout;
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

        std::unique_ptr<Pipeline> m_pipeline;

        std::vector<std::unique_ptr<Buffer>> m_uniformBuffers;
        std::vector<VkDescriptorSet> m_descriptorSets{};

    };
}


#endif //DEPTHPREPASS_H
