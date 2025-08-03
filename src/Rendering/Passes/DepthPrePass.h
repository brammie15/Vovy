#ifndef DEPTHPREPASS_H
#define DEPTHPREPASS_H
#include "Core/Device.h"
#include "Descriptors/DescriptorPool.h"

#include <glm/glm.hpp>

#include "Resources/Image.h"
#include "Resources/UniformBuffer.h"
#include "Scene/Scene.h"
#include "Utils/FrameContext.h"

namespace vov {
    class DepthPrePass final {
    public:

        explicit DepthPrePass(Device& deviceRef) : m_device{deviceRef}, m_uniformBuffer{deviceRef} {}
        ~DepthPrePass();

        void Init(VkFormat depthFormat, uint32_t framesInFlight);
        void Record(const FrameContext& context, Image& depthImage);

        void Resize(VkExtent2D newSize);

        struct alignas(16) UniformBufferData
        {
            glm::mat4 view;
            glm::mat4 proj;
        };
        struct PushConstant
        {
            glm::mat4 model;
            int objectId;
        };

    private:
        Device& m_device;
        VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};
        uint32_t m_framesInFlight{1};

        std::unique_ptr<DescriptorPool> m_descriptorPool;

        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout;
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

        std::unique_ptr<Pipeline> m_pipeline;

        UniformBuffer<UniformBufferData> m_uniformBuffer;

        std::vector<VkDescriptorSet> m_descriptorSets{};
    };
}


#endif //DEPTHPREPASS_H
