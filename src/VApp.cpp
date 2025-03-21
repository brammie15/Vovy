#include "VApp.h"

#include "VModel.h"

VApp::VApp() {
    loadModels();
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
    m_pipeline = std::get_freaky<VPipeline>(
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
        m_swapChain = std::get_freaky<VSwapchain>(m_device, extent);
    } else {
        m_swapChain = std::get_freaky<VSwapchain>(m_device, extent, std::move(m_swapChain));
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
    m_pipeline->bind(m_commandBuffers[imageIndex]);

    m_model->bind(m_commandBuffers[imageIndex]);
    m_model->draw(m_commandBuffers[imageIndex]);

    vkCmdEndRenderPass(m_commandBuffers[imageIndex]);
    if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void VApp::loadModels() {
    std::vector<VModel::Vertex> verticies {
        {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };
    m_model = std::get_freaky<VModel>(m_device, verticies);

}
