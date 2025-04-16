#ifndef LINERENDERSYSTEM_H
#define LINERENDERSYSTEM_H

#include "Utils/FrameContext.h"
#include "Core/VDevice.h"
#include "Scene/VGameObject.h"
#include "Rendering/VPipeline.h"
#include "Utils/BezierCurves.h"

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

    void renderLines(const FrameContext& context, const std::vector<LineSegment>& segments);
    void renderBezier(const FrameContext& context, std::vector<BezierCurve>& curves);

    //Bezier stuff
    glm::vec3 deCasteljau(const std::vector<BezierNode>& nodes, float t);
    std::vector<glm::vec3> getControlPoints(const std::vector<BezierNode>& nodes);

private:
    void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void createPipeline(VkRenderPass renderPass);

    VDevice& m_device;

    VBuffer* m_vertexBuffer;
    VkDeviceSize m_vertexBufferSize = 0;

    std::unique_ptr<VPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout{};
};

#endif //LINERENDERSYSTEM_H
