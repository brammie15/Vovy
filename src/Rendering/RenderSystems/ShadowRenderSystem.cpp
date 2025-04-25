#include "ShadowRenderSystem.h"

#include <stdexcept>

#include "GameObjectRenderSystem.h"
#include "Utils/DebugLabel.h"
#include "Utils/Timer.h"

vov::ShadowRenderSystem::ShadowRenderSystem(Device& deviceRef, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts): m_device(deviceRef) {
    createPipelineLayout(descriptorSetLayouts);
    createPipeline();

    Timer timer("Creating shadow map");
    m_renderTexture = std::make_unique<RenderTexture>(
        m_device,
        1024 * 8,
        1024 * 8,
        VK_FORMAT_D32_SFLOAT,
        true
    );
    timer.stop();
}

vov::ShadowRenderSystem::~ShadowRenderSystem() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void vov::ShadowRenderSystem::render(const FrameContext& frameContext, vov::Scene& scene) const {

    DebugLabel::ScopedCmdLabel label(
        frameContext.commandBuffer,
        "ShadowRenderSystem",
        {0.0f, 1.0f, 0.0f, 1.0f}
    );

    m_renderTexture->beginRendering(frameContext.commandBuffer);
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
    for (auto& obj: scene.getGameObjects()) {
        obj->model->draw(frameContext.commandBuffer, m_pipelineLayout);
    }
    m_renderTexture->endRendering(frameContext.commandBuffer);
}

VkDescriptorImageInfo vov::ShadowRenderSystem::getShadowMapDescriptorInfo() const {
    return m_renderTexture->getTargetImage().descriptorInfo();
}

void vov::ShadowRenderSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
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

    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void vov::ShadowRenderSystem::createPipeline() {
    assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.colorAttachments = {};
    // pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;



    pipelineConfig.pipelineLayout = m_pipelineLayout;
    m_pipeline = std::make_unique<Pipeline>(
        m_device,
        "shaders/shadow.vert.spv",
        "shaders/shadow.frag.spv",
        pipelineConfig
    );
}
