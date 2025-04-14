#ifndef LINERENDERSYSTEM_H
#define LINERENDERSYSTEM_H

#include "Utils/FrameContext.h"
#include "Core/VDevice.h"
#include "Scene/VGameObject.h"
#include "VPipeline.h"

struct LineSegment {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
};

//TODO: fix this
struct AnotherPushConstantData {
    glm::mat4 modelMatrix{1.f};
};


class LineRenderSystem {
public:
    LineRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);
    ~LineRenderSystem();


    void renderLines(FrameContext context, const std::vector<LineSegment>& segments);
private:
    void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void createPipeline(VkRenderPass renderPass);

    VDevice& m_device;

    VBuffer* m_vertexBuffer;

    std::unique_ptr<VPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout{};
};

#endif //LINERENDERSYSTEM_H
