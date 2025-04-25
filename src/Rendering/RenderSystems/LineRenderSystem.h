#ifndef LINERENDERSYSTEM_H
#define LINERENDERSYSTEM_H

#include "Core/Device.h"
#include "Rendering/Pipeline.h"
#include "Scene/GameObject.h"
#include "Utils/BezierCurves.h"
#include "Utils/FrameContext.h"

namespace vov {
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
        LineRenderSystem(Device& deviceRef, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);
        ~LineRenderSystem();

        void renderLines(const FrameContext& context, const std::vector<LineSegment>& segments);
        void renderBezier(const FrameContext& context, const std::vector<BezierCurve>& curves);

        //Bezier stuff
        glm::vec3 deCasteljau(const std::vector<BezierNode>& nodes, float t);
        std::vector<glm::vec3> getControlPoints(const std::vector<BezierNode>& nodes);

    private:
        void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
        void createPipeline();

        Device& m_device;

        Buffer* m_vertexBuffer;
        VkDeviceSize m_vertexBufferSize = 0;

        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout{};
    };
}
#endif //LINERENDERSYSTEM_H
