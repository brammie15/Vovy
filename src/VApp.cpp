#include "VApp.h"

#include "VModel.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct PushConstantData {
    glm::mat2 transform{1.f};
    glm::vec2 offset;
    alignas(16) glm::vec3 color;
};

struct GlobalUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

VApp::VApp() {
    loadGameObjects();
    createPipelineLayout();
    recreateSwapChain();
    createCommandBuffers();
}

VApp::~VApp() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void VApp::run() {
    while (!m_window.ShouldClose()) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(m_device.device());
}

void VApp::createPipelineLayout() {

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);


    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void VApp::createPipeline() {
    assert(m_swapChain != nullptr && "Cannot create pipeline before swap chain");
    assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
    VPipelineConfigInfo pipelineConfig{};

    VPipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.renderPass = m_swapChain->getRenderPass();
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    m_pipeline = std::make_unique<VPipeline>(
    m_device, "shaders/shader.vert.spv", "shaders/shader.frag.spv", pipelineConfig
    );
}

void VApp::createCommandBuffers() {
    m_commandBuffers.resize(m_swapChain->imageCount());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device.getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device.device(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // for (int i{ 0 }; i < m_commandBuffers.size(); ++i) {
    //
    // }
}

void VApp::drawFrame() {
    uint32_t imageIndex;
    auto result = m_swapChain->acquireNextImage(&imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    recordCommandBuffer(imageIndex);
    result = m_swapChain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasWindowResized()) {
        m_window.resetWindowResized();
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void VApp::recreateSwapChain() {
    auto extent = m_window.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = m_window.getExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(m_device.device());

    if (m_swapChain == nullptr) {
        m_swapChain = std::make_unique<VSwapchain>(m_device, extent);
    } else {
        m_swapChain = std::make_unique<VSwapchain>(m_device, extent, std::move(m_swapChain));
        if (m_swapChain->imageCount() != m_commandBuffers.size()) {
            freeCommandBuffers();
            createCommandBuffers();
        }
    }

    createPipeline();
}


void VApp::freeCommandBuffers() {
    vkFreeCommandBuffers(
        m_device.device(),
        m_device.getCommandPool(),
        static_cast<uint32_t>(m_commandBuffers.size()),
        m_commandBuffers.data());
    m_commandBuffers.clear();
}

void VApp::renderGameObjects(VkCommandBuffer commandbuffer) {
    m_pipeline->bind(commandbuffer);
    for (auto& obj: m_gameObjects) {
        PushConstantData push{};
        push.offset = obj.transform.translation;
        push.color = obj.color;
        // push.transform = obj.transform.mat2();
        push.transform = glm::mat2{1.0f};

        vkCmdPushConstants(commandbuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &push);
        obj.model->bind(commandbuffer);
        obj.model->draw(commandbuffer);
    }
}

void VApp::recordCommandBuffer(int imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain->getRenderPass();
    renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(imageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain->GetSwapChainExtent();

    VkClearValue clearValues[2] = {
        {0.1f, 0.1f, 0.1f, 1.0f},
        {1.0f, 0}
    };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(std::size(clearValues));
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
    viewport.height = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, m_swapChain->GetSwapChainExtent()};
    vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);

    renderGameObjects(m_commandBuffers[imageIndex]);

    vkCmdEndRenderPass(m_commandBuffers[imageIndex]);
    if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void VApp::loadGameObjects() {
    std::vector<VModel::Vertex> vertices {
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}, // Top-left
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Bottom-left
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right
        {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}  // Top-right
    };

    std::vector<uint32_t> indices {
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
    };
    auto m_model = std::make_shared<VModel>(m_device, vertices, indices);

    auto test = VGameObject::createGameObject();
    test.model = m_model;
    test.color = {1.0f, 0.0f, 0.0f};
    test.transform.translation.x = 0.2f;

    m_gameObjects.push_back(std::move(test));

}
