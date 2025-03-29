#ifndef VRENDERSYSTEM_H
#define VRENDERSYSTEM_H
#include "Utils/FrameContext.h"
#include "Core/VDevice.h"
#include "Scene/VGameObject.h"
#include "VPipeline.h"

struct PushConstantData {
    glm::mat2 transform{1.f};
    glm::vec2 offset;
    alignas(16) glm::vec3 color;
};

class VRenderSystem {
public:
    VRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
    ~VRenderSystem();

    void renderGameObjects(FrameContext& frameContext, std::vector<VGameObject>& gameObjects);
private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

    VDevice& m_device;

    std::unique_ptr<VPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};

#endif //VRENDERSYSTEM_H
