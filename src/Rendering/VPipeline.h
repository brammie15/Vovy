#ifndef VPIPELINE_H
#define VPIPELINE_H

#include <string>
#include <vector>

#include "Core/VDevice.h"

struct VPipelineConfigInfo {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    VkPipelineViewportStateCreateInfo viewportInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class VPipeline {
public:
    VPipeline(VDevice& device, const std::string& vertPath, const std::string& fragPath, const VPipelineConfigInfo& configInfo);
    ~VPipeline();

    VPipeline(const VPipeline& other) = delete;
    VPipeline(VPipeline&& other) noexcept = delete;
    VPipeline& operator=(const VPipeline& other) = delete;
    VPipeline& operator=(VPipeline&& other) noexcept = delete;

    void bind(VkCommandBuffer buffer);
    static void DefaultPipelineConfigInfo(VPipelineConfigInfo& configInfo);

private:
    static std::vector<char> readFile(const std::string& filename);

    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath, const VPipelineConfigInfo& configInfo);

    void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

    VDevice& m_device;

    VkPipeline m_graphicsPipeline{VK_NULL_HANDLE};
    VkShaderModule m_vertShaderModule{VK_NULL_HANDLE};
    VkShaderModule m_fragShaderModule{VK_NULL_HANDLE};
};

#endif //VPIPELINE_H
