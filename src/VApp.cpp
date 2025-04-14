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
#include "Utils/DeltaTime.h"


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


    while (!m_window.ShouldClose()) {
        DeltaTime::GetInstance().Update();
        glfwPollEvents();

        m_imguiRenderSystem.beginFrame();

        this->imGui();

        m_imguiRenderSystem.endFrame();


        if (glfwGetKey(VWindow::gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(VWindow::gWindow, GLFW_TRUE);
        }

        //TODO: remove
        camera.Update(DeltaTime::GetInstance().GetDeltaTime());

        camera.setAspectRatio(m_renderPass.GetAspectRatio());
        camera.CalculateProjectionMatrix();
        camera.CalculateViewMatrix();

        camera.Translate(glm::vec3(0, 0, 0));

        if (auto commandBuffer = m_renderPass.BeginFrame()) {
            int frameIndex = m_renderPass.GetFrameIndex();
            FrameContext frameInfo{frameIndex, static_cast<float>(DeltaTime::GetInstance().GetDeltaTime()), commandBuffer, globalDescriptorSets[frameIndex]};

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
        auto sleepTime = DeltaTime::GetInstance().SleepDuration();
        if (sleepTime > std::chrono::nanoseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
    vkDeviceWaitIdle(m_device.device());
}

void VApp::imGui() {
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", 1.0f / DeltaTime::GetInstance().GetDeltaTime());
    ImGui::Text("Frame Time: %.3f ms", DeltaTime::GetInstance().GetDeltaTime() * 1000.0f);
    ImGui::Text("Window Size: %d x %d", m_window.getWidth(), m_window.getHeight());
    ImGui::End();

    ImGui::Begin("Object 1 Transform");
    glm::vec3 translation = m_gameObjects[1]->transform.GetLocalPosition();
    glm::quat rotation = m_gameObjects[1]->transform.GetLocalRotation();
    glm::vec3 scale = m_gameObjects[1]->transform.GetLocalScale();

    if (ImGui::DragFloat3("Translation", &translation[0], 0.1f)) {
        m_gameObjects[1]->transform.SetLocalPosition(translation);
    }

    if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
        m_gameObjects[1]->transform.SetLocalRotation(rotation);
    }

    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
        m_gameObjects[1]->transform.SetLocalScale(scale);
    }

    ImGui::End();
}

void VApp::loadGameObjects() {
    auto sponzaScene = VGameObject::createGameObject();

    // const auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/NewSponza_Main_glTF_003.gltf");
    const auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/sponza.obj", sponzaScene.get());

    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/NewSponza_Main_Yup_003.fbx");

    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/MonkeyTest.fbx");
    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/MonkeyTest.obj");
    // auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/viking_room.obj");

    sponzaScene->model = std::move(mainSponzaModel);

    m_gameObjects.push_back(std::move(sponzaScene));

    auto testObject = VGameObject::createGameObject();
    const auto testModel = std::make_shared<VModel>(m_device, "resources/cat.obj", testObject.get());
    testObject->model = std::move(testModel);
    testObject->transform.SetLocalPosition({0.0f, 0.0f, 0.0f});

    m_gameObjects.push_back(std::move(testObject));

    // auto curtainsSponzaModel = std::make_shared<VModel>(m_device, "resources/pkg_a_curtains/NewSponza_Curtains_glTF.gltf");
    // auto curtainsSponza = VGameObject::createGameObject();
    // curtainsSponza->model = std::move(curtainsSponzaModel);
    // m_gameObjects.push_back(std::move(curtainsSponza));
}
