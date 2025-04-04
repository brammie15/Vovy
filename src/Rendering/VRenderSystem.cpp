#include "VRenderSystem.h"

#include <stdexcept>

VRenderSystem::VRenderSystem(VDevice& deviceRef, VkRenderPass renderPass,  const std::vector<VkDescriptorSetLayout> descriptorSetLayout): m_device{deviceRef} {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);
}

VRenderSystem::~VRenderSystem() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void VRenderSystem::renderGameObjects(FrameContext& frameContext, std::vector<std::unique_ptr<VGameObject>>& gameObjects) {
    m_pipeline->bind(frameContext.commandBuffer);
    // auto projectionView = frameContext.camera.getProjection() * frameInfo.camera.getView();

    vkCmdBindDescriptorSets(
        frameContext.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &frameContext.globalDescriptor,
        0,
        nullptr
    );

    for (auto& obj : gameObjects) {
        // PushConstantData push{};
        // push.color = obj.color;
        // auto modelMatrix = obj.transform.mat4();
        // push.transform = projectionView * modelMatrix;
        // push.normalMatrix = obj.transform.normalMatrix();

        // vkCmdPushConstants(
        //     frameContext.commandBuffer,
        //     m_pipelineLayout,
        //     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        //     0,
        //     sizeof(PushConstantData),
        //     &push);
        // obj.model->bind(frameContext.commandBuffer);
        obj->model->draw(frameContext.commandBuffer, m_pipelineLayout);
    }

}

void VRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout> descriptorSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{descriptorSetLayout};

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

void VRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    VPipelineConfigInfo pipelineConfig{};
    VPipeline::DefaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    m_pipeline = std::make_unique<VPipeline>(
        m_device,
        "shaders/shader.vert.spv",
        "shaders/shader.frag.spv",
        pipelineConfig
    );
}

