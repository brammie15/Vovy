#ifndef BLITPASS_H
#define BLITPASS_H
#include "LightingPass.h"
#include "Core/Device.h"
#include "Descriptors/DescriptorPool.h"
#include "Rendering/Swapchain.h"
#include "Utils/FrameContext.h"

namespace vov {
    class DescriptorSetLayout;
    class Pipeline;

    struct UniformBuffer {
        Camera::CameraSettings camSettings{};
    };

    class BlitPass {
    public:
        explicit BlitPass(Device& deviceRef, uint32_t framesInFlight, LightingPass& lightingPass, Swapchain& swapchain);
        ~BlitPass();

        void Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, const Swapchain& swapchain);

        void UpdateDescriptor(uint32_t frameIndex, Image& lightingOutput);

        //TODO: not the best in consistancy
        void Resize(VkExtent2D newSize, LightingPass& lightingPass);

    private:
        Device& m_device;
        LightingPass& m_lightingPass;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        uint32_t m_framesInFlight{};

        std::vector<VkDescriptorSet> m_descriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};
        std::vector<std::unique_ptr<Buffer>> m_exposureBuffers{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;
    };
}


#endif //BLITPASS_H
