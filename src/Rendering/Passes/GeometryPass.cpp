#include "GeometryPass.h"

#include <array>

#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Utils/DebugLabel.h"
#include "Utils/ResourceManager.h"

vov::GeometryPass::GeometryPass(vov::Device& deviceRef, const CreateInfo& createInfo): m_device{deviceRef} {
    m_geoBuffers.resize(createInfo.maxFrames);
    for (auto & buffer : m_geoBuffers) {
        buffer = std::make_unique<GeoBuffer>(deviceRef, createInfo.size);
    }

    m_descriptorPool = DescriptorPool::Builder(m_device)
            .setMaxSets(createInfo.maxFrames * 3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, createInfo.maxFrames * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 5)
            .build();

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    //TODO: make it so this is like more global ish since i now manually match the models / mesh defintion
    m_textureSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //Albedo texture
        .build();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstant);

    const std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout(),
        m_textureSetLayout->getDescriptorSetLayout()
    };

    m_uniformBuffers.resize(createInfo.maxFrames);
    for (size_t i{ 0 }; i < m_uniformBuffers.size(); i++) {
        m_uniformBuffers[i] = std::make_unique<Buffer>(
            m_device, sizeof(UniformBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, true
        );
    }

    m_descriptorSets.resize(createInfo.maxFrames);

    for (size_t i{0}; i < createInfo.maxFrames; i++) {
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

    pipelineConfig.colorAttachments = m_geoBuffers[0]->GetFormats();
    pipelineConfig.depthAttachment = m_depthFormat;
    //TODO: fix this to not be static
    uint32_t gBufferAttachmentCount = 4;

    VkPipelineColorBlendAttachmentState colorBlendAttachemnt = pipelineConfig.colorBlendAttachment;
    std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments = {
        colorBlendAttachemnt,
        colorBlendAttachemnt,
        colorBlendAttachemnt,
        colorBlendAttachemnt
    };

    pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_EQUAL;


    pipelineConfig.colorBlendInfo.attachmentCount = gBufferAttachmentCount;
    pipelineConfig.colorBlendInfo.pAttachments = colorBlendAttachments.data();

    pipelineConfig.depthAttachment = VK_FORMAT_D32_SFLOAT;


    m_pipeline = std::make_unique<Pipeline>(
        m_device,
        "shaders/deferred.vert.spv",
        "shaders/deferred.frag.spv",
        pipelineConfig
    );

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = deviceRef.getProperties().limits.maxSamplerAnisotropy;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    VkResult result = vkCreateSampler(deviceRef.device(), &samplerInfo, nullptr, &m_TextureSampler);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
    }
    //
    // VkDescriptorImageInfo imageInfo{};
    // imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // imageInfo.imageView = testImg->getImageView();
    // imageInfo.sampler = m_TextureSampler;
    //
    // DescriptorWriter(*m_textureSetLayout, *m_descriptorPool)
    //     .writeImage(0, &imageInfo)
    //     .build(m_textureSet);


}

vov::GeometryPass::~GeometryPass() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
    vkDestroySampler(m_device.device(), m_TextureSampler, nullptr);
}

void vov::GeometryPass::Record(const FrameContext& context, VkCommandBuffer commandBuffer, int imageIndex, Image& depthImage, Scene* scene, Camera* Camera) {
    UniformBuffer ubo;
    ubo.view = Camera->GetViewMatrix();
    ubo.proj = Camera->GetProjectionMatrix();
    ubo.proj[1][1] *= -1;

    m_uniformBuffers[imageIndex]->copyTo(&ubo, sizeof(ubo));
    m_uniformBuffers[imageIndex]->flush();

    m_geoBuffers[imageIndex]->TransitionWriting(commandBuffer);

    auto& gBufferAttachments = m_geoBuffers[imageIndex]->GetRenderingAttachments();
    uint32_t gBufferAttachmentCount = m_geoBuffers[imageIndex]->GetRenderAttachmentCount();
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImage.getImageView();
    depthAttachment.imageLayout = depthImage.getCurrentLayout();
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Render Info
    VkExtent2D extent = m_geoBuffers[imageIndex]->GetExtent();
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, extent };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = gBufferAttachmentCount;
    renderingInfo.pColorAttachments = gBufferAttachments.data();
    renderingInfo.pDepthAttachment = &depthAttachment;
    renderingInfo.pStencilAttachment = nullptr;

    DebugLabel::BeginCmdLabel(commandBuffer, "Geometrypass", glm::vec4{1.f, 0, 0 ,1.f});
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

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &m_textureSet, 0, nullptr);

    m_pipeline->bind(commandBuffer);

    for (const auto& object : scene->getGameObjects()) {

        object->model->draw(commandBuffer, m_pipelineLayout);
    }

    vkCmdEndRendering(commandBuffer);
    DebugLabel::EndCmdLabel(commandBuffer);
}
