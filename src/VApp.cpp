#include "VApp.h"

#include "Scene/VModel.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

#include "Utils/FrameContext.h"
#include "Utils/VCamera.h"
#include "Resources/VImage.h"
#include "Rendering/VRenderSystem.h"
#include "Descriptors/VDescriptorSetLayout.h"
#include "Descriptors/VDescriptorWriter.h"


struct GlobalUBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

VApp::VApp() {
    m_globalPool =
      VDescriptorPool::Builder(m_device)
          .setMaxSets(VSwapchain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VSwapchain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VSwapchain::MAX_FRAMES_IN_FLIGHT)
          .build();
    loadGameObjects();
}

VApp::~VApp() = default;

void VApp::run() {

    VImage textureImage{m_device, "resources/cat.jpg", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY};

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
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(VSwapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto imageInfo = textureImage.descriptorInfo();
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        auto success = VDescriptorWriter(*globalSetLayout, *m_globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo)
            .build(globalDescriptorSets[i]);

        if (!success) {
            throw std::runtime_error("failed to write descriptor sets!");
        }
    }


    VRenderSystem renderSystem{m_device, m_renderPass.GetSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    float time = 0.0f;

    VCamera camera{{-2.0f, 1.0f, 0}, {0.0f, 1.0f, 0.0f}};


    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!m_window.ShouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        time += frameTime;

        camera.Update(frameTime);

        camera.setAspectRatio(m_renderPass.GetAspectRatio());
        camera.CalculateProjectionMatrix();
        camera.CalculateViewMatrix();

        camera.Translate(glm::vec3(0 ,0, 0));

        if (auto commandBuffer = m_renderPass.BeginFrame()) {
            int frameIndex = m_renderPass.GetFrameIndex();
            FrameContext frameInfo{frameIndex, frameTime, commandBuffer, globalDescriptorSets[frameIndex]};

            GlobalUBO ubo{};
            // ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));            // ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            ubo.model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), time, glm::vec3{0, 1.f, 0});
            // ubo.view = glm::lookAt(glm::vec3(2.f, 0, 0), glm::vec3(1.0f, 0.f, 0.f), glm::vec3(0.0f, 0.0f, 1.0f));

            camera.CalculateViewMatrix();
            ubo.view = camera.GetViewMatrix();

            ubo.proj = camera.GetProjectionMatrix();

            ubo.proj[1][1] *= -1;

            uboBuffers[frameIndex]->copyTo(&ubo, sizeof(ubo));
            uboBuffers[frameIndex]->flush();

            m_renderPass.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(frameInfo, m_gameObjects);
            m_renderPass.endSwapChainRenderPass(commandBuffer);
            m_renderPass.endFrame();
        }
    }
    vkDeviceWaitIdle(m_device.device());
}

void VApp::loadGameObjects() {
    // std::vector<VModel::Vertex> vertices{
    //     {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}, // Top-left
    //     {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Bottom-left
    //     {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right
    //     {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}} // Top-right
    // };
    //
    // std::vector<uint32_t> indices{
    //     0, 1, 2, // First triangle
    //     2, 3, 0 // Second triangle
    // };

    // auto m_model = std::make_shared<VModel>(m_device, vertices, indices);
    auto model = VModel::createModelFromFile(m_device, "resources/cat.obj");

    auto test = VGameObject::createGameObject();
    test.model = std::move(model);
    test.color = {1.0f, 0.0f, 0.0f};
    // test.transform.translation.x

    m_gameObjects.push_back(std::move(test));


    //Make a plane 1x1 wide
    std::vector<VModel::Vertex> vertices{
        {{-0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}}, // Top-left
        {{-0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // Bottom-left
        {{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right
        {{0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}} // Top-right
    };

    std::vector<uint32_t> indices{
        0, 1, 2, // First triangle
        2, 3, 0 // Second triangle
    };

    auto test2 = VGameObject::createGameObject();
    test2.model = std::make_shared<VModel>(m_device, vertices, indices);
    test2.color = {0.0f, 1.0f, 0.0f};
    test2.transform.translation = {0.0f, 0.0f, 0.0f};
    test2.transform.scale = {1.0f, 1.0f, 1.0f};

    m_gameObjects.push_back(std::move(test2));

}
