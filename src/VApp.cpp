#include "VApp.h"

#include "VModel.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <chrono>
#include <glm/glm.hpp>

#include "FrameContext.h"
#include "VRenderSystem.h"
#include "Descriptors/VDescriptorSetLayout.h"
#include "Descriptors/VDescriptorWriter.h"


struct GlobalUBO {
    // glm::mat4 view;
    // glm::mat4 proj;
    glm::vec3 color{};
    glm::vec2 offset{};
};

VApp::VApp() {
    m_globalPool =
      VDescriptorPool::Builder(m_device)
          .setMaxSets(VSwapchain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VSwapchain::MAX_FRAMES_IN_FLIGHT)
          .build();
    loadGameObjects();
}

VApp::~VApp() {
}

void VApp::run() {
    std::vector<std::unique_ptr<VBuffer>> uboBuffers(VSwapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<VBuffer>(
            m_device,
            sizeof(GlobalUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = VDescriptorSetLayout::Builder(m_device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(VSwapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        VDescriptorWriter(*globalSetLayout, *m_globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }


    VRenderSystem renderSystem{m_device, m_renderPass.GetSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!m_window.ShouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;


        if (auto commandBuffer = m_renderPass.BeginFrame()) {
            int frameIndex = m_renderPass.GetFrameIndex();
            // FrameContext frameInfo{frameIndex, frameTime, commandBuffer, camera};
            FrameContext frameInfo{frameIndex, frameTime, commandBuffer, globalDescriptorSets[frameIndex]};

            // update
            GlobalUBO ubo{};
            ubo.color = {0.0f, 1.0f, 0.0f};
            // ubo.projectionView = camera.getProjection() * camera.getView();
            uboBuffers[frameIndex]->copyTo(&ubo, sizeof(ubo));
            uboBuffers[frameIndex]->flush();

            // render
            m_renderPass.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(frameInfo, m_gameObjects);
            m_renderPass.endSwapChainRenderPass(commandBuffer);
            m_renderPass.endFrame();
        }
    }
    vkDeviceWaitIdle(m_device.device());
}


void VApp::loadGameObjects() {
    std::vector<VModel::Vertex> vertices{
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}, // Top-left
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Bottom-left
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right
        {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}} // Top-right
    };

    std::vector<uint32_t> indices{
        0, 1, 2, // First triangle
        2, 3, 0 // Second triangle
    };
    auto m_model = std::make_shared<VModel>(m_device, vertices, indices);

    auto test = VGameObject::createGameObject();
    test.model = m_model;
    test.color = {1.0f, 0.0f, 0.0f};
    test.transform.translation.x = 0.2f;

    m_gameObjects.push_back(std::move(test));
}
