#include "SelectPass.h"

#include <stdexcept>

vov::SelectPass::SelectPass(Device& device, VkExtent2D extent): m_device{device}, m_extent{extent} {

    m_descriptorPool = DescriptorPool::Builder(m_device)
               .setMaxSets(4)
               .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 2)
               .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)
               .build();

    VkPushConstantRange pushConstandRange{};
    pushConstandRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstandRange.offset = 0;
    pushConstandRange.size = sizeof(PushConstant);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;;
    pipelineLayoutInfo.pSetLayouts = nullptr;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstandRange;
    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not make pipeline layout");
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.pipelineLayout = m_pipelineLayout;

    pipelineConfig.vertexAttributeDescriptions = {};
    pipelineConfig.vertexBindingDescriptions = {};


}
