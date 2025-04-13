#include "VApp.h"

#include "Scene/VMesh.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <chrono>
#include <thread>
#include <glm/gtx/string_cast.hpp>

#include "Utils/FrameContext.h"
#include "Utils/VCamera.h"
#include "Rendering/VRenderSystem.h"
#include "Descriptors/VDescriptorSetLayout.h"
#include "Descriptors/VDescriptorWriter.h"
#include "Rendering/ImguiRenderSystem.h"


struct GlobalUBO {
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

    auto modelSetLayout = VDescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(VSwapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        auto success = VDescriptorWriter(*globalSetLayout, *m_globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);

        if (!success) {
            throw std::runtime_error("failed to write descriptor sets!");
        }
    }


    VRenderSystem renderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        {globalSetLayout->getDescriptorSetLayout(), modelSetLayout->getDescriptorSetLayout()}
    };

    ImguiRenderSystem m_imguiRenderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        static_cast<int>(m_window.getWidth()),
        static_cast<int>(m_window.getHeight())
    };

    float time = 0.0f;
    VCamera camera{{-2.0f, 1.0f, 0}, {0.0f, 1.0f, 0.0f}};


    const int targetFPS = 60;
    const std::chrono::duration<float> targetFrameDuration(1.0f / targetFPS);

    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!m_window.ShouldClose()) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();
        glfwPollEvents();

        m_imguiRenderSystem.beginFrame();

        // Build your GUI
        ImGui::Begin("Hello, Vulkan!");
        ImGui::Text("This is ImGui with Vulkan!");
        ImGui::End();

        m_imguiRenderSystem.endFrame();


        if (glfwGetKey(VWindow::gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(VWindow::gWindow, GLFW_TRUE);
        }

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        time += frameTime;

        camera.Update(frameTime);

        camera.setAspectRatio(m_renderPass.GetAspectRatio());
        camera.CalculateProjectionMatrix();
        camera.CalculateViewMatrix();

        camera.Translate(glm::vec3(0, 0, 0));

        if (auto commandBuffer = m_renderPass.BeginFrame()) {
            int frameIndex = m_renderPass.GetFrameIndex();
            FrameContext frameInfo{frameIndex, frameTime, commandBuffer, globalDescriptorSets[frameIndex]};

            GlobalUBO ubo{};

            camera.CalculateViewMatrix();
            ubo.view = camera.GetViewMatrix();

            ubo.proj = camera.GetProjectionMatrix();

            ubo.proj[1][1] *= -1;

            uboBuffers[frameIndex]->copyTo(&ubo, sizeof(ubo));
            uboBuffers[frameIndex]->flush();

            m_renderPass.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(frameInfo, m_gameObjects);

            m_imguiRenderSystem.renderImgui(commandBuffer);
            m_renderPass.endSwapChainRenderPass(commandBuffer);
            m_renderPass.endFrame();
        }

        // FPS limiter
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto elapsed = frameEndTime - frameStartTime;

        if (elapsed < targetFrameDuration) {
            std::this_thread::sleep_for(targetFrameDuration - elapsed);
        }
    }
    vkDeviceWaitIdle(m_device.device());
}

void VApp::loadGameObjects() {
    // const auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/NewSponza_Main_glTF_003.gltf");
    const auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/sponza.obj");

    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/NewSponza_Main_Yup_003.fbx");

    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/MonkeyTest.fbx");
    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/MonkeyTest.obj");
    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/viking_room.obj");

    auto sponzaScene = VGameObject::createGameObject();
    sponzaScene->model = std::move(mainSponzaModel);

    m_gameObjects.push_back(std::move(sponzaScene));

    // auto curtainsSponzaModel = std::make_shared<VModel>(m_device, "resources/pkg_a_curtains/NewSponza_Curtains_glTF.gltf");

    // auto curtainsSponza = VGameObject::createGameObject();
    // curtainsSponza->model = std::move(curtainsSponzaModel);

    // m_gameObjects.push_back(std::move(curtainsSponza));


    //
    // //Make a plane 1x1 wide
    // std::vector<VMesh::Vertex> vertices{
    //     {{-0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}}, // Top-left
    //     {{-0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // Bottom-left
    //     {{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right
    //     {{0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}} // Top-right
    // };
    //
    // std::vector<uint32_t> indices{
    //     0, 1, 2, // First triangle
    //     2, 3, 0 // Second triangle
    // };
    //
    // auto test2 = VGameObject::createGameObject();
    //
    //
    // std::vector<VMesh::Builder> builders;
    // auto model1 = VMesh::Builder{vertices, indices};
    // builders.push_back(std::move(model1));
    // test2->model = std::make_shared<VModel>(m_device, builders);
    // test2->color = {0.0f, 1.0f, 0.0f};
    // // test2.transform.translation = {0.0f, 0.0f, 0.0f};
    // // test2.transform.scale = {1.0f, 1.0f, 1.0f};
    //
    // m_gameObjects.push_back(std::move(test2));
}
