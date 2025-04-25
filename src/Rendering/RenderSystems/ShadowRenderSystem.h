#ifndef SHADOWRENDERSYSTEM_H
#define SHADOWRENDERSYSTEM_H

#include <memory>

#include "Core/Device.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderTexture.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Utils/FrameContext.h"

namespace vov {
    class ShadowRenderSystem {
    public:
        explicit ShadowRenderSystem(Device& deviceRef, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

        ~ShadowRenderSystem();

        void render(const FrameContext& frameContext, vov::Scene& scene) const;
        [[nodiscard]] VkDescriptorImageInfo getShadowMapDescriptorInfo() const;

    private:
        Device& m_device;
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout{};
        VkRenderPass m_renderPass{};
        VkDescriptorSetLayout m_descriptorSetLayout{};

        std::unique_ptr<RenderTexture> m_renderTexture;

        void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
        void createPipeline();
    };
}


#endif //SHADOWRENDERSYSTEM_H
