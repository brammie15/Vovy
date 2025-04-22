#include "LineRenderSystem.h"

#include <stdexcept>
namespace vov {
    LineRenderSystem::LineRenderSystem(Device& deviceRef, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout): m_device{deviceRef} {
        createPipelineLayout(descriptorSetLayout);
        createPipeline(renderPass);

        m_vertexBufferSize = sizeof(Mesh::Vertex) * 100;
        m_vertexBuffer = new Buffer{
            m_device,
            m_vertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        };
    }

    LineRenderSystem::~LineRenderSystem() {
        delete m_vertexBuffer;
        vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    }

    void LineRenderSystem::renderLines(const FrameContext& context, const std::vector<LineSegment>& segments) {
        std::vector<Mesh::Vertex> vertexData;
        vertexData.reserve(segments.size() * 2);

        for (const auto& segment: segments) {
            vertexData.push_back({segment.start, segment.color});
            vertexData.push_back({segment.end, segment.color});
        }

        const VkDeviceSize requiredSize = sizeof(Mesh::Vertex) * vertexData.size();

        // Resize buffer if needed
        if (requiredSize > m_vertexBufferSize) {
            delete m_vertexBuffer;

            m_vertexBufferSize = requiredSize * 2;

            m_vertexBuffer = new Buffer{
                m_device,
                m_vertexBufferSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU
            };
        }

        m_vertexBuffer->map();
        m_vertexBuffer->copyTo(vertexData.data(), requiredSize);
        m_vertexBuffer->unmap();

        m_pipeline->bind(context.commandBuffer);

        vkCmdBindDescriptorSets(
            context.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &context.globalDescriptor,
            0,
            nullptr
        );

        AnotherPushConstantData push{};
        push.modelMatrix = glm::mat4(1.f);

        vkCmdPushConstants(context.commandBuffer,
                           m_pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(AnotherPushConstantData),
                           &push
        );

        const VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
        constexpr VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, buffers, offsets);

        vkCmdDraw(context.commandBuffer, static_cast<uint32_t>(vertexData.size()), 1, 0, 0);
    }

    void LineRenderSystem::renderBezier(const FrameContext& context, const std::vector<BezierCurve>& curves) {
        if (curves.empty()) {
            return;
        }

        std::vector<Mesh::Vertex> vertexData;

        for (auto& curve: curves) {
            if (curve.nodes.empty()) {
                continue;
            }

            for (int i = 0; i < curve.resolution; ++i) {
                float t = static_cast<float>(i) / (curve.resolution - 1);
                glm::vec3 point = deCasteljau(curve.nodes, t);

                vertexData.push_back({point, curve.color});
            }

            for (auto& node: curve.nodes) {
                vertexData.push_back({node.position, glm::vec3(0.0f, 0.0f, 1.0f)}); // Blue color for control points
            }
        }

        if (vertexData.empty()) {
            return;
        }

        const VkDeviceSize requiredSize = sizeof(Mesh::Vertex) * vertexData.size();

        if (requiredSize > m_vertexBufferSize) {
            delete m_vertexBuffer;

            // Optional: grow buffer a bit more than required to reduce reallocations
            m_vertexBufferSize = requiredSize * 2;

            m_vertexBuffer = new Buffer{
                m_device,
                m_vertexBufferSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU
            };
        }

        m_vertexBuffer->map();
        m_vertexBuffer->copyTo(vertexData.data(), requiredSize);
        m_vertexBuffer->unmap();

        m_pipeline->bind(context.commandBuffer);

        vkCmdBindDescriptorSets(
            context.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &context.globalDescriptor,
            0,
            nullptr
        );

        AnotherPushConstantData push{};
        push.modelMatrix = glm::mat4(1.f);

        vkCmdPushConstants(context.commandBuffer,
                           m_pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(AnotherPushConstantData),
                           &push
        );

        const VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
        constexpr VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, buffers, offsets);

        vkCmdDraw(context.commandBuffer, static_cast<uint32_t>(vertexData.size()), 1, 0, 0);
    }

    //https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm
    glm::vec3 LineRenderSystem::deCasteljau(const std::vector<BezierNode>& nodes, float t) {
        std::vector<glm::vec3> points = getControlPoints(nodes);

        while (points.size() > 1) {
            std::vector<glm::vec3> nextPoints;
            for (size_t i = 0; i < points.size() - 1; ++i) {
                nextPoints.push_back((1 - t) * points[i] + t * points[i + 1]);
            }
            points = std::move(nextPoints);
        }

        return points[0];
    }

    std::vector<glm::vec3> LineRenderSystem::getControlPoints(const std::vector<BezierNode>& nodes) {
        std::vector<glm::vec3> controlPoints;
        for (auto& node: nodes) {
            controlPoints.push_back(node.position);
        }
        return controlPoints;
    }

    void LineRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(AnotherPushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
            }
    }

    void LineRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

        pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;


        // pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = m_pipelineLayout;
        m_pipeline = std::make_unique<Pipeline>(
            m_device,
            "shaders/line.vert.spv",
            "shaders/line.frag.spv",
            pipelineConfig
        );
    }
}