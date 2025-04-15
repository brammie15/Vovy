#ifndef VRENDERSYSTEM_H
#define VRENDERSYSTEM_H
#include "../../Utils/FrameContext.h"
#include "../../Core/VDevice.h"
#include "../../Scene/VGameObject.h"
#include "../VPipeline.h"

struct PushConstantData {
    glm::mat4 modelMatrix{1.f};
};

class GameObjectRenderSystem {
public:
    GameObjectRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);
    ~GameObjectRenderSystem();

    void renderGameObjects(const FrameContext& frameContext, const std::vector<std::unique_ptr<VGameObject>>& gameObjects) const;
private:
    void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void createPipeline(VkRenderPass renderPass);

    VDevice& m_device;

    std::unique_ptr<VPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout{};
};

#endif //VRENDERSYSTEM_H
