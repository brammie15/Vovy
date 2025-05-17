#include "BlitPass.h"

#include <array>

#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Utils/DebugLabel.h"
#include "Utils/ResourceManager.h"

vov::BlitPass::BlitPass(Device& deviceRef, uint32_t framesInFlight, LightingPass& lightingPass, Swapchain& swapchain): m_device{deviceRef}, m_lightingPass{lightingPass}, m_framesInFlight{framesInFlight} {
    m_descriptorPool = DescriptorPool::Builder(m_device)
            .setMaxSets(framesInFlight * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, framesInFlight * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * 2)
            .build();

    m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) //exposure ubo
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    const std::array descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout()
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = 0;

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

    VkFormat swapchainFormat = swapchain.GetSwapChainImageFormat();

    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.colorAttachments = {swapchainFormat};

    m_pipeline = std::make_unique<Pipeline>(
        m_device,
        "shaders/triangle.vert.spv",
        "shaders/blit.frag.spv",
        pipelineConfig
    );

    m_descriptorSets.resize(framesInFlight);
    m_exposureBuffers.resize(framesInFlight);

    auto dummyImage = ResourceManager::GetInstance().loadImage(m_device, "resources/Gear.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    for (size_t i{0}; i < m_framesInFlight; i++) {
        m_descriptorPool->allocateDescriptor(m_descriptorSetLayout->getDescriptorSetLayout(), m_descriptorSets[i]);

        m_exposureBuffers[i] = std::make_unique<Buffer>(
            m_device, sizeof(ExposureUbo),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, true
        );

        auto bufferInfo = m_exposureBuffers[i]->descriptorInfo();


        VkDescriptorImageInfo dummyInfo{};
        dummyInfo.sampler = lightingPass.GetImage(i).getSampler();
        dummyInfo.imageView = lightingPass.GetImage(i).getImageView();
        dummyInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &dummyInfo)
                .build(m_descriptorSets[i]);
    }
}

vov::BlitPass::~BlitPass() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void vov::BlitPass::Record(const FrameContext& context, VkCommandBuffer commandBuffer, uint32_t imageIndex, const Swapchain& swapchain) {
    const Image& currentImage = swapchain.GetImage(static_cast<int>(imageIndex));

    ExposureUbo ubo{};
    ubo.exposure = context.camera.GetExposure();

    m_exposureBuffers[imageIndex]->copyTo(&ubo, sizeof(ExposureUbo));

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = currentImage.getImageView();
    colorAttachment.imageLayout = currentImage.getCurrentLayout();
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.f, 0.f, 0.f, 1.0f}};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, currentImage.getExtent()};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    DebugLabel::BeginCmdLabel(commandBuffer, "Blitting pass", glm::vec4(1.f, 0.7f, 0.1f, 1));

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(currentImage.getExtent().width);
    viewport.height = static_cast<float>(currentImage.getExtent().height);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = currentImage.getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    m_pipeline->bind(commandBuffer);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    DebugLabel::EndCmdLabel(commandBuffer);
}

void vov::BlitPass::UpdateDescriptor(uint32_t frameIndex, Image& lightingOutput) {
    auto imageInfo = lightingOutput.descriptorInfo();
    DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
            .writeImage(0, &imageInfo)
            .overwrite(m_descriptorSets[frameIndex]);
}

void vov::BlitPass::Resize(VkExtent2D newSize, LightingPass& lightingPass) {
    for (size_t i{0}; i < m_framesInFlight; i++) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = lightingPass.GetImage(i).getSampler();
        imageInfo.imageView = lightingPass.GetImage(i).getImageView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        auto bufferInfo = m_exposureBuffers[i]->descriptorInfo();

        DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &imageInfo)
                .overwrite(m_descriptorSets[i]);
    }
}
