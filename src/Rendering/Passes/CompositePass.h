#ifndef COMPOSITEPASS_H
#define COMPOSITEPASS_H
#include "GeometryPass.h"
#include "Core/Device.h"
#include "Rendering/Swapchain.h"
#include "Utils/FrameContext.h"

namespace vov {
    class CompositePass {
    public:

        struct PushConstant {
            glm::mat4 model;
        };

        explicit CompositePass(Device& deviceRef) : m_device{deviceRef} {}
        ~CompositePass();

        void Init(uint32_t framesInFlight, const Swapchain& swapchain);

        void Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, const GeometryPass& geoPass, const Swapchain& swapchain);

        void UpdateDescriptors(uint32_t frameIndex, Image& albedo, Image& normal, Image& specular, Image& bump);
    private:

        Device& m_device;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        uint32_t m_framesInFlight{};

        std::unique_ptr<DescriptorSetLayout> m_geobufferSamplersSetLayout{};
        std::vector<VkDescriptorSet> m_textureDescriptors{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;


    };
}


#endif //COMPOSITEPASS_H
