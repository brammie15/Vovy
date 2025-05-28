#include "ShadowPass.h"

#include <array>
#include <stdexcept>

#include "Descriptors/DescriptorWriter.h"
#include "Resources/Buffer.h"
#include "Utils/DebugLabel.h"

vov::ShadowPass::ShadowPass(Device& deviceRef, uint32_t framesInFlight, VkFormat format, VkExtent2D extent, DirectionalLight& directionalLight): m_device{deviceRef}, m_framesInFlight{framesInFlight}, m_imageFormat{format}, m_directionalLight{directionalLight} {
    m_descriptorPool = DescriptorPool::Builder(m_device)
            .setMaxSets(framesInFlight * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, framesInFlight * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * 2)
            .build();

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    m_descriptorSets.resize(framesInFlight);

    m_depthImage = std::make_unique<Image>(
            m_device,
            extent,
            format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_AUTO,
            true,
            true
    );
    m_depthImage->SetName("ShadowDepthImage");

    m_uniformBuffers.resize(framesInFlight);
    for (size_t i{0}; i < m_framesInFlight; i++) {
        m_uniformBuffers[i] = std::make_unique<Buffer>(
            m_device, sizeof(UniformBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, true
        );
    }

    for (size_t i{0}; i < m_framesInFlight; i++) {
        auto bufferInfo = m_uniformBuffers[i]->descriptorInfo();
        DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .build(m_descriptorSets[i]);
    }

    const std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout()
    };

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstant);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.pipelineLayout = m_pipelineLayout;

    pipelineConfig.colorAttachments = {};
    pipelineConfig.depthAttachment = m_depthImage->GetFormat();

    pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
    pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;  // Common for shadows

    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;


    m_pipeline = std::make_unique<Pipeline>(
       m_device,
       "shaders/depthPrepass.vert.spv",
       "",
       pipelineConfig
   );


}

vov::ShadowPass::~ShadowPass() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void vov::ShadowPass::Record(const FrameContext& context) {
    const uint32_t imageIndex = context.frameIndex;
    const auto commandBuffer = context.commandBuffer;

    UniformBuffer ubo{};
    ubo.lightViewMatrix = m_directionalLight.GetViewMatrix();
    ubo.lightProjectionMatrix = m_directionalLight.GetProjectionMatrix();

    ubo.lightProjectionMatrix[1][1] *= -1;

    m_uniformBuffers[imageIndex]->copyTo(&ubo, sizeof(UniformBuffer));
    m_uniformBuffers[imageIndex]->flush();

    m_depthImage->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = m_depthImage->GetImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { .depth = 1.0f, .stencil = 0 };

    const VkExtent2D extent = m_depthImage->GetExtent();
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, extent};
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachment;

    DebugLabel::BeginCmdLabel(
        commandBuffer,
        "Shadow Pass",
        {1.0f, 0.2f, 0.7f, 1.0f}
    );

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = { .x = 0, .y = 0 };
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    m_pipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    for (const auto& object : context.currentScene.getGameObjects()) {

        object->model->draw(commandBuffer, m_pipelineLayout, true);
    }

    vkCmdEndRendering(commandBuffer);

    m_depthImage->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    DebugLabel::EndCmdLabel(commandBuffer);
}

void vov::ShadowPass::Resize(VkExtent2D newSize) {
    m_depthImage.reset();
    m_depthImage = std::make_unique<Image>(
            m_device,
            newSize,
            m_imageFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_AUTO,
            true,
            true
    );
    m_depthImage->SetName("ShadowDepthImage");
}

vov::Image& vov::ShadowPass::GetDepthImage(int frameIndex) {
    return *m_depthImage;
}
