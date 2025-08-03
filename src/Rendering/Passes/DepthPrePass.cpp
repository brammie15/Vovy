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
}

void vov::DepthPrePass::Init(VkFormat depthFormat, uint32_t framesInFlight) {
    //Cant save device
    m_depthFormat = depthFormat;
    m_framesInFlight = framesInFlight;

    m_descriptorPool = DescriptorPool::Builder(m_device)
        .setMaxSets(framesInFlight * 2)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * 2)
        .build();

    DebugLabel::SetObjectName(
        reinterpret_cast<uint64_t>(m_descriptorPool->GetHandle()),
        VK_OBJECT_TYPE_DESCRIPTOR_POOL,
        "Depth Pre Pass Descriptor Pool"
    );

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    m_uniformBuffer.SetName("Depth Pre Pass Uniform Buffer");


    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstant);


    const std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout()
    };

    m_descriptorSets.resize(framesInFlight);

    for (size_t i{0}; i < framesInFlight; i++) {
        m_descriptorPool->allocateDescriptor(m_descriptorSetLayout->getDescriptorSetLayout(), m_descriptorSets[i]);
        auto bufferInfo = m_uniformBuffer[i]->descriptorInfo();
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
    pipelineConfig.name = "Depth Pre Pass Pipeline";

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

void vov::DepthPrePass::Record(const FrameContext& context, Image& depthImage) {
    const uint32_t imageIndex = context.frameIndex;
    const auto commandBuffer = context.commandBuffer;

    UniformBufferData ubo{};
    ubo.view = context.camera.GetViewMatrix();
    ubo.proj = context.camera.GetProjectionMatrix();
    ubo.proj[1][1] *= -1;

    m_uniformBuffer.update(imageIndex, ubo);

    depthImage.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImage.GetImageView();
    depthAttachment.imageLayout = depthImage.GetCurrentLayout(); //God this bad
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { .depth = 1.0f, .stencil = 0 };

    // Render Info
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, depthImage.GetExtent() };
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
    viewport.width = static_cast<float>(depthImage.GetExtent().width);
    viewport.height = static_cast<float>(depthImage.GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    DebugLabel::BeginCmdLabel(commandBuffer, "Viewport and scissor", {0.0f, 1.0f, 0.0f, 1.0f});
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = depthImage.GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    DebugLabel::EndCmdLabel(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    m_pipeline->bind(commandBuffer);

    for (const auto& object : context.currentScene.getGameObjects()) {
        for (const auto& mesh : object->model->getMeshes()) {
            PushConstant push{};
            push.model = mesh->getTransform().GetWorldMatrix();

            vkCmdPushConstants(
                commandBuffer,
                m_pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstant),
                &push
            );

            mesh->bind(commandBuffer);
            mesh->draw(commandBuffer);
        }
    }

    vkCmdEndRendering(commandBuffer);

    depthImage.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    DebugLabel::EndCmdLabel(commandBuffer);
}

void vov::DepthPrePass::Resize(VkExtent2D newSize) {
    //Swapchain handles
}
