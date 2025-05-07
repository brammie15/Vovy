#include "DepthPrePass.h"

#include <array>
#include <stdexcept>

#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Rendering/Pipeline.h"
#include "Resources/Buffer.h"
#include "Utils/DebugLabel.h"

vov::DepthPrePass::~DepthPrePass() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    m_descriptorPool.reset();
    m_descriptorSetLayout.reset();
    m_pipeline.reset();
    m_uniformBuffers.clear();
}

void vov::DepthPrePass::Init(VkFormat depthFormat, uint32_t framesInFlight) {
    //Cant save device
    m_depthFormat = depthFormat;
    m_framesInFlight = framesInFlight;


    m_descriptorPool = DescriptorPool::Builder(m_device)
        .setMaxSets(framesInFlight * 2)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * 2)
        .build();

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build();


    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstant);

    std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout()
    };

    m_uniformBuffers.resize(framesInFlight);
    for (size_t i{ 0 }; i < m_uniformBuffers.size(); i++) {
        m_uniformBuffers[i] = std::make_unique<Buffer>(
            m_device, sizeof(UniformBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, true
        );
    }

    m_descriptorSets.resize(framesInFlight);

    for (size_t i{0}; i < framesInFlight; i++) {
        m_descriptorPool->allocateDescriptor(m_descriptorSetLayout->getDescriptorSetLayout(), m_descriptorSets[i]);
        auto bufferInfo = m_uniformBuffers[i]->descriptorInfo();
        DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
            .writeBuffer(0, &bufferInfo)
            .build(m_descriptorSets[i]);
    }

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

    //Making the pipeline
    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.pipelineLayout = m_pipelineLayout;

    pipelineConfig.colorAttachments = {};
    pipelineConfig.depthAttachment = m_depthFormat;

    m_pipeline = std::make_unique<Pipeline>(
        m_device,
        "shaders/depthPrepass.vert.spv",
        "",
        pipelineConfig
    );

}

void vov::DepthPrePass::Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, Image& depthImage, Scene* scene, Camera* camera) {
    UniformBuffer ubo;
    ubo.view = camera->GetViewMatrix();
    ubo.proj = camera->GetProjectionMatrix();

    m_uniformBuffers[imageIndex]->copyTo(&ubo, sizeof(ubo));
    m_uniformBuffers[imageIndex]->flush();

    depthImage.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImage.getImageView();
    depthAttachment.imageLayout = depthImage.getCurrentLayout();; //God this bad
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { .depth = 1.0f, .stencil = 0 };

    // Render Info
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, depthImage.getExtent() };
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachment;

    DebugLabel::BeginCmdLabel(
        commandBuffer,
        "Depth Pre Pass",
        {0.0f, 0.0f, 1.0f, 1.0f}
    );

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(depthImage.getExtent().width);
    viewport.height = static_cast<float>(depthImage.getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    DebugLabel::BeginCmdLabel(commandBuffer, "Viewport and scissor", {0.0f, 1.0f, 0.0f, 1.0f});
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = depthImage.getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    DebugLabel::EndCmdLabel(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    m_pipeline->bind(commandBuffer);

    for (const auto& object : scene->getGameObjects()) {
        object->model->draw(commandBuffer, m_pipelineLayout);
    }

    vkCmdEndRendering(commandBuffer);

    depthImage.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    DebugLabel::EndCmdLabel(commandBuffer);
}
