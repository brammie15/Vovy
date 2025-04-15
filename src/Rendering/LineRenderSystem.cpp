#include "LineRenderSystem.h"

#include <stdexcept>

LineRenderSystem::LineRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout): m_device{deviceRef} {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);

    m_vertexBufferSize = sizeof(VMesh::Vertex) * 100;
    m_vertexBuffer = new VBuffer{
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

void LineRenderSystem::renderLines(FrameContext context, const std::vector<LineSegment>& segments) {

    std::vector<VMesh::Vertex> vertexData;
    vertexData.reserve(segments.size() * 2);

    for (const auto& segment : segments) {
        vertexData.push_back({segment.start, segment.color});
        vertexData.push_back({segment.end, segment.color});
    }

    VkDeviceSize requiredSize = sizeof(VMesh::Vertex) * vertexData.size();

    // Resize buffer if needed
    if (requiredSize > m_vertexBufferSize) {
        delete m_vertexBuffer;

        // Optional: grow buffer a bit more than required to reduce reallocations
        m_vertexBufferSize = requiredSize * 2;

        m_vertexBuffer = new VBuffer{
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

    VkBuffer buffers[] = { m_vertexBuffer->getBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, buffers, offsets);

    vkCmdDraw(context.commandBuffer, static_cast<uint32_t>(vertexData.size()), 1, 0, 0);
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

    VPipelineConfigInfo pipelineConfig{};
    VPipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;


    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    m_pipeline = std::make_unique<VPipeline>(
        m_device,
        "shaders/line.vert.spv",
        "shaders/line.frag.spv",
        pipelineConfig
    );
}
