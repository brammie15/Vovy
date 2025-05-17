#include "LightingPass.h"

#include <array>
#include <sstream>

#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Utils/DebugLabel.h"
#include "Utils/ResourceManager.h"

vov::LightingPass::LightingPass(Device& deviceRef, uint32_t framesInFlight, VkFormat format, VkExtent2D extent): m_device{deviceRef}, m_framesInFlight{framesInFlight}, m_imageFormat{format} {
    m_descriptorPool = DescriptorPool::Builder(m_device)
            .setMaxSets(framesInFlight * 4)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, framesInFlight * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * 2)
            .build();

    m_geobufferSamplersSetLayout = DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //albedo
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //normal
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //Specular
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //MetallicRoughness
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //Selection
            .build();

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    m_uniformBuffers.resize(m_framesInFlight);
    for (size_t i{0}; i < m_uniformBuffers.size(); i++) {
        m_uniformBuffers[i] = std::make_unique<Buffer>(
            m_device, sizeof(UniformBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, true
        );
    }

    m_descriptorSets.resize(m_framesInFlight);
    for (size_t i{0}; i < m_framesInFlight; i++) {
        m_descriptorPool->allocateDescriptor(m_descriptorSetLayout->getDescriptorSetLayout(), m_descriptorSets[i]);
        auto bufferInfo = m_uniformBuffers[i]->descriptorInfo();
        DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .build(m_descriptorSets[i]);
    }


    VkPushConstantRange pushConstandRange{};
    pushConstandRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstandRange.offset = 0;
    pushConstandRange.size = sizeof(PushConstant);

    const std::array descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout(),
        m_geobufferSamplersSetLayout->getDescriptorSetLayout()
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstandRange;
    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not make pipleine layout");
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.vertexBindingDescriptions = {};
    pipelineConfig.vertexAttributeDescriptions = {};

    //No depthBuffer
    pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    // pipelineConfig.depthAttachment = VK_FORMAT_UNDEFINED;

    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.colorAttachments = {format};
    // pipelineConfig.depthAttachment

    m_pipeline = std::make_unique<Pipeline>(
        m_device,
        "shaders/triangle.vert.spv",
        "shaders/lightingPass.frag.spv",
        pipelineConfig
    );

    m_textureDescriptors.resize(framesInFlight);

    auto dummyImage = ResourceManager::GetInstance().loadImage(m_device, "resources/Gear.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    for (uint32_t i{0}; i < framesInFlight; ++i) {
        VkDescriptorImageInfo dummyInfo{};
        dummyInfo.sampler = dummyImage->getSampler();
        dummyInfo.imageView = dummyImage->getImageView();
        dummyInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


        DescriptorWriter writer(*m_geobufferSamplersSetLayout, *m_descriptorPool);
        writer.writeImage(0, &dummyInfo);
        writer.writeImage(1, &dummyInfo);
        writer.writeImage(2, &dummyInfo);
        writer.writeImage(3, &dummyInfo);
        writer.writeImage(4, &dummyInfo);

        if (!writer.build(m_textureDescriptors[i])) {
            throw std::runtime_error("Failed to build descriptor set for CompositePass");
        }
    }

    m_renderTargets.resize(framesInFlight);
    for (int index{0}; index < framesInFlight; ++index) {
        m_renderTargets[index] = std::make_unique<Image>(
            m_device,
            extent.width, extent.height,
            format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VK_FILTER_NEAREST
        );
        m_renderTargets[index]->SetName("LightTarget" + index);
    }
}

vov::LightingPass::~LightingPass() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void vov::LightingPass::Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, const GeometryPass& geoPass, Scene& scene) {
    auto& currentImage = m_renderTargets[imageIndex];

    auto cameraPos = glm::vec4(context.camera.m_position, 0);
    UniformBuffer ubo{};
    ubo.camSettings.cameraPos = cameraPos;
    ubo.camSettings.exposure = context.camera.GetExposure();

    ubo.lightInfo.direction = scene.GetDirectionalLight().GetDirection();
    ubo.lightInfo.color = scene.GetDirectionalLight().GetColor();
    ubo.lightInfo.intensity = scene.GetDirectionalLight().GetIntensity();

    m_uniformBuffers[imageIndex]->copyTo(&ubo, sizeof(UniformBuffer));

    m_renderTargets[imageIndex]->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = currentImage->getImageView();
    colorAttachment.imageLayout = currentImage->getCurrentLayout();
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.f, 0.f, 0.f, 1.0f}};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, currentImage->getExtent()};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    DebugLabel::BeginCmdLabel(commandBuffer, "Lighting pass", glm::vec4(0.5f, 0.7f, 0.1f, 1));
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(currentImage->getExtent().width);
    viewport.height = static_cast<float>(currentImage->getExtent().height);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = currentImage->getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &m_textureDescriptors[imageIndex], 0, nullptr);

    m_pipeline->bind(commandBuffer);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    m_renderTargets[imageIndex]->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    DebugLabel::EndCmdLabel(commandBuffer);
}

void vov::LightingPass::UpdateDescriptors(uint32_t frameIndex, Image& albedo, Image& normal, Image& specular, Image& bump) {
    DescriptorWriter writer(*m_geobufferSamplersSetLayout, *m_descriptorPool);

    const auto albedoInfo = albedo.descriptorInfo();
    const auto normalInfo = normal.descriptorInfo();
    const auto specularInfo = specular.descriptorInfo();
    const auto bumpInfo = bump.descriptorInfo();

    writer
            .writeImage(0, &albedoInfo)
            .writeImage(1, &normalInfo)
            .writeImage(2, &specularInfo)
            .writeImage(3, &bumpInfo)
            .overwrite(m_textureDescriptors[frameIndex]);
}

vov::Image& vov::LightingPass::GetImage(int imageIndex) const {
    return *m_renderTargets[imageIndex];
}

void vov::LightingPass::Resize(VkExtent2D newSize) {
    m_renderTargets.clear();
    m_renderTargets.resize(m_framesInFlight);

    for (int index{0}; index < m_framesInFlight; ++index) {
        m_renderTargets[index] = std::make_unique<Image>(
            m_device,
            newSize.width, newSize.height,
            m_imageFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_AUTO
        );
        m_renderTargets[index]->SetName("LightTarget" + index);
    }
}
