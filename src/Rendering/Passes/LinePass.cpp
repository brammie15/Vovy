#include "LinePass.h"

#include <array>
#include <stdexcept>

#include "Descriptors/DescriptorWriter.h"
#include "Resources/Buffer.h"
#include "Utils/DebugLabel.h"
#include "Utils/LineManager.h"



vov::LinePass::LinePass(Device& deviceRef, uint32_t framesInFlight, VkExtent2D extent): m_device{deviceRef}, m_framesInFlight{framesInFlight} {
    m_descriptorPool = DescriptorPool::Builder(m_device)
        .setMaxSets(framesInFlight * 2)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * 2)
        .build();

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    for (size_t i{0}; i < m_framesInFlight; i++) {
        m_uniformBuffers.emplace_back(std::make_unique<Buffer>(
            m_device, sizeof(UniformBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, true
        ));
    }

    m_descriptorSets.resize(m_framesInFlight);
    for (size_t i{0}; i < m_framesInFlight; i++) {
        auto bufferInfo = m_uniformBuffers[i]->descriptorInfo();
        DescriptorWriter writer(*m_descriptorSetLayout, *m_descriptorPool);
        writer.writeBuffer(0, &bufferInfo);
    }

    m_renderTargets.resize(m_framesInFlight);
    for (int index{0}; index < m_framesInFlight; ++index) {
        m_renderTargets[index] = std::make_unique<Image>(
            m_device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
    }

    PipelineConfigInfo pipelineInfo{};
    Pipeline::DefaultPipelineConfigInfo(pipelineInfo);

    pipelineInfo.vertexAttributeDescriptions = LineManager::Vertex::GetAttributeDescriptions();
    pipelineInfo.vertexBindingDescriptions = LineManager::Vertex::GetBindingDescriptions();

    pipelineInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    const std::array descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout()
    };

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

vov::LinePass::~LinePass() {
}

void vov::LinePass::Record(FrameContext context, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    Image& renderTarget = *m_renderTargets[imageIndex];

    UpdateVertexBuffer();

    auto cameraPos = context.camera.GetPosition();
    UniformBuffer uniformData{};
    uniformData.viewProjectionMatrix = context.camera.GetViewProjectionMatrix();
    uniformData.cameraPosition = glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z);

    m_uniformBuffers[imageIndex]->copyTo(&uniformData, sizeof(UniformBuffer));

    renderTarget.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = renderTarget.GetImageView();
    colorAttachment.imageLayout = renderTarget.GetCurrentLayout();
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.f, 0.f, 0.f, 1.0f}};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, renderTarget.GetExtent()};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    DebugLabel::BeginCmdLabel(commandBuffer, "Line pass", glm::vec4(235.f / 255.f, 106.f / 255.f, 14.f / 255.f, 1));
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderTarget.GetExtent().width);
    viewport.height = static_cast<float>(renderTarget.GetExtent().height);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = renderTarget.GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    m_pipeline->bind(commandBuffer);

    const VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
    const VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(LineManager::GetInstance().GetLines().size() * 2), 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    renderTarget.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    DebugLabel::EndCmdLabel(commandBuffer);
}

void vov::LinePass::UpdateVertexBuffer() {
    std::vector<LineManager::Line> lines = LineManager::GetInstance().GetLines();

    std::vector<LineManager::Vertex> vertices;
    vertices.reserve(lines.size() * 2);
    for (const auto& line : lines) {
        vertices.push_back({line.start.position, line.start.color});
        vertices.push_back({line.end.position, line.end.color});
    }

    Buffer stagingBuffer{
        m_device,
        sizeof(LineManager::Vertex) * vertices.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    };

    stagingBuffer.map();
    stagingBuffer.copyTo(vertices.data(), sizeof(LineManager::Vertex) * vertices.size());
    stagingBuffer.unmap();

    m_device.copyBuffer(
        &stagingBuffer,
        m_vertexBuffer.get(),
        sizeof(LineManager::Vertex) * vertices.size()
    );
}
