#ifndef VRENDERSYSTEM_H
#define VRENDERSYSTEM_H
#include "Utils/FrameContext.h"
#include "Core/VDevice.h"
#include "Scene/VGameObject.h"
#include "VPipeline.h"

struct PushConstantData {
    glm::mat4 modelMatrix{1.f};
};

class VRenderSystem {
public:
    VRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout> descriptorSetLayout);
    ~VRenderSystem();

    void renderGameObjects(FrameContext& frameContext, std::vector<std::unique_ptr<VGameObject>>& gameObjects);
private:
    void createPipelineLayout(const std::vector<VkDescriptorSetLayout> descriptorSetLayout);
    void createPipeline(VkRenderPass renderPass);

    VDevice& m_device;

    std::unique_ptr<VPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};

#endif //VRENDERSYSTEM_H
