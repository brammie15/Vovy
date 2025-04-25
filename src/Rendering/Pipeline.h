#ifndef VPIPELINE_H
#define VPIPELINE_H

#include <string>
#include <vector>

#include "Core/Device.h"

namespace vov {
    struct PipelineConfigInfo {
        VkPipelineViewportStateCreateInfo viewportInfo{};
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;

        VkPipelineRenderingCreateInfoKHR renderingInfo{};
        std::vector<VkFormat> colorAttachments{};

        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};

        VkPipelineLayout pipelineLayout = nullptr;
    };

    class Pipeline {
    public:
        Pipeline(Device& device, const std::string& vertPath, const std::string& fragPath, const PipelineConfigInfo& configInfo);
        ~Pipeline();

        Pipeline(const Pipeline& other) = delete;
        Pipeline(Pipeline&& other) noexcept = delete;
        Pipeline& operator=(const Pipeline& other) = delete;
        Pipeline& operator=(Pipeline&& other) noexcept = delete;

        void bind(VkCommandBuffer buffer) const;
        static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

    private:
        static std::vector<char> readFile(const std::string& filename);

        void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath, const PipelineConfigInfo& configInfo);

        void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) const;

        Device& m_device;

        VkPipeline m_graphicsPipeline{VK_NULL_HANDLE};
        VkShaderModule m_vertShaderModule{VK_NULL_HANDLE};
        VkShaderModule m_fragShaderModule{VK_NULL_HANDLE};
    };
}

#endif //VPIPELINE_H
