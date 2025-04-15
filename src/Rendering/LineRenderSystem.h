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

struct BezierNode {
    glm::vec3 position;
};

struct BezierCurve {
    std::vector<BezierNode> nodes;
    glm::vec3 color;
    int resolution;
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
    void renderBezier(FrameContext context, const std::vector<BezierCurve>& curves);
private:
    void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void createPipeline(VkRenderPass renderPass);

    //Bezier stuff
    glm::vec3 deCasteljau(const std::vector<BezierNode>& nodes, float t);
    std::vector<glm::vec3> getControlPoints(const std::vector<BezierNode>& nodes);

    VDevice& m_device;

    VBuffer* m_vertexBuffer;
    VkDeviceSize m_vertexBufferSize = 0;

    std::unique_ptr<VPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout{};
};

#endif //LINERENDERSYSTEM_H
