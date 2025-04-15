#include "GameObjectRenderSystem.h"

#include <stdexcept>

GameObjectRenderSystem::GameObjectRenderSystem(VDevice& deviceRef, VkRenderPass renderPass,  const std::vector<VkDescriptorSetLayout>& descriptorSetLayout): m_device{deviceRef} {
    createPipelineLayout(descriptorSetLayout);
    createPipeline(renderPass);
}

GameObjectRenderSystem::~GameObjectRenderSystem() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void GameObjectRenderSystem::renderGameObjects(const FrameContext& frameContext, const std::vector<std::unique_ptr<VGameObject>>& gameObjects) {
    m_pipeline->bind(frameContext.commandBuffer);

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
        obj->model->draw(frameContext.commandBuffer, m_pipelineLayout);
    }

}

void GameObjectRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

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

void GameObjectRenderSystem::createPipeline(VkRenderPass renderPass) {
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

