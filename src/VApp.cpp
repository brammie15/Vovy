#include "VApp.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <chrono>
#include <math.h>

#define _USE_MATH_DEFINES
#include <functional>
#include <iostream>
#include <math.h>
#include <thread>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Utils/FrameContext.h"
#include "Utils/VCamera.h"
#include "Rendering/RenderSystems/GameObjectRenderSystem.h"
#include "Descriptors/VDescriptorSetLayout.h"
#include "Descriptors/VDescriptorWriter.h"
#include "Rendering/RenderSystems/ImguiRenderSystem.h"
#include "Rendering/RenderSystems/LineRenderSystem.h"
#include "Utils/DeltaTime.h"
#include "Scene/VMesh.h"

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

    m_sigmaVanniScene = std::make_unique<VScene>("SigmaVanniScene");
    m_sponzaScene = std::make_unique<VScene>("SponzaScene");
    m_vikingRoomScene = std::make_unique<VScene>("VikingRoomScene");

    loadGameObjects();
    m_currentScene = m_sigmaVanniScene.get();
    m_currentScene->sceneLoad();

    std::vector<BezierNode> controlPoints = {
        BezierNode{glm::vec3(0.0f, 0.0f, 0.0f)},
        BezierNode{glm::vec3(1.0f, 2.0f, 0.0f)},
        BezierNode{glm::vec3(2.0f, -1.0f, 0.0f)},
        BezierNode{glm::vec3(3.0f, 0.0f, 0.0f)}
    };
    //
    m_sigmaVanniScene->addBezierCurve(BezierCurve{
    controlPoints,
    glm::vec3(1.0f, 0.0f, 0.0f),  // Red color
    100
    });
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

    GameObjectRenderSystem renderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        {globalSetLayout->getDescriptorSetLayout(), modelSetLayout->getDescriptorSetLayout()}
    };

    ImguiRenderSystem imguiRenderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        static_cast<int>(m_window.getWidth()),
        static_cast<int>(m_window.getHeight())
    };

    LineRenderSystem lineRenderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        { globalSetLayout->getDescriptorSetLayout(), modelSetLayout->getDescriptorSetLayout() }
    };

    VCamera camera{{-2.0f, 1.0f, 0}, {0.0f, 1.0f, 0.0f}};
    camera.setAspectRatio(m_renderPass.GetAspectRatio());

    while (!m_window.ShouldClose()) {
        DeltaTime::GetInstance().Update();
        m_window.PollInput();

        imguiRenderSystem.beginFrame();

        this->imGui();
        if (m_currentScene->getGameObjects().size() > 0 && m_selectedTransform) {
            imguiRenderSystem.drawGizmos(&camera, m_selectedTransform, "Maintransform");
        }

        auto& curves = m_currentScene->getBezierCurves();
        for (int i = 0; i < curves.size(); i++) {
            for (int j = 0; j < curves[i].nodes.size(); j++) {
                std::string id = "BezierNode" + std::to_string(j);
                imguiRenderSystem.drawGizmos(&camera, curves[i].nodes[j].position, id);
            }
        }

        imguiRenderSystem.endFrame();

        if (m_window.isKeyPressed(GLFW_KEY_GRAVE_ACCENT)) {
            if (m_window.isCursorLocked()) {
                m_window.UnlockCursor();
            } else {
                m_window.LockCursor();
            }
        }

        if (m_window.isKeyPressed(GLFW_KEY_P)) {
            if (m_currentScene->getLineSegments().size() == 0) {
                m_currentScene->addLineSegment(LineSegment{
                    {0.0f, 0.0f, 0.0f},
                    {1.0f, 1.0f, 1.0f},
                    {1.f, 1.f, 0.f}
                });
            } else {
                //Add the cameras postion to the line segments the start should be the previous line segment
                const auto& lastSegment = m_currentScene->getLineSegments().back();
                m_currentScene->addLineSegment(LineSegment{
                    lastSegment.end,
                    camera.m_position,
                    {1.f, 1.f, 0.f}
                });
            }
        }

        //TODO: remove
        camera.Update(DeltaTime::GetInstance().GetDeltaTime());

        camera.CalculateProjectionMatrix();
        camera.CalculateViewMatrix();

        if (auto commandBuffer = m_renderPass.BeginFrame()) {
            int frameIndex = m_renderPass.GetFrameIndex();
            FrameContext frameInfo{frameIndex, static_cast<float>(DeltaTime::GetInstance().GetDeltaTime()), commandBuffer, globalDescriptorSets[frameIndex], camera};

            GlobalUBO ubo{};

            camera.CalculateViewMatrix();
            ubo.view = camera.GetViewMatrix();

            ubo.proj = camera.GetProjectionMatrix();

            ubo.proj[1][1] *= -1;

            uboBuffers[frameIndex]->copyTo(&ubo, sizeof(ubo));
            uboBuffers[frameIndex]->flush();

            m_renderPass.beginSwapChainRenderPass(commandBuffer);

            if (m_currentScene->getLineSegments().size() > 2) {
                lineRenderSystem.renderLines(frameInfo, m_currentScene->getLineSegments());
            }

            lineRenderSystem.renderBezier(frameInfo, m_currentScene->getBezierCurves());

            renderSystem.renderGameObjects(frameInfo, m_currentScene->getGameObjects());

            imguiRenderSystem.renderImgui(commandBuffer);
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
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("New"))
            {
                // Action for New
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", 1.0f / DeltaTime::GetInstance().GetDeltaTime());
    ImGui::Text("Frame Time: %.3f ms", DeltaTime::GetInstance().GetDeltaTime() * 1000.0f);
    ImGui::Text("Window Size: %d x %d", m_window.getWidth(), m_window.getHeight());
    ImGui::End();

    ImGui::Begin("Line Segments");
    ImGui::Text("Line Segments: %d", m_currentScene->getLineSegments().size());
    for (int i = 0; i < m_currentScene->getLineSegments().size(); i++) {
        std::string label = "Line Segment " + std::to_string(i);
        if (ImGui::BeginCombo(label.c_str(), "Line Segment")) {
            ImGui::DragFloat3("Start", &m_currentScene->getLineSegments()[i].start.x, 0.1f);
            ImGui::DragFloat3("End", &m_currentScene->getLineSegments()[i].end.x, 0.1f);
            ImGui::EndCombo();
        }
    }
    ImGui::End();


    ImGui::Begin("Bezier Segments");
    if (m_currentScene->getBezierCurves().size() > 0) {
        ImGui::Text("Bezier Segments: %d", m_currentScene->getBezierCurves()[0].nodes.size());
        if (ImGui::Button("add Node")) {
            glm::vec3 prevNodePosition = m_currentScene->getBezierCurves()[0].nodes.back().position;
            m_currentScene->getBezierCurves()[0].nodes.push_back(BezierNode{prevNodePosition + glm::vec3(0.0f, 1.0f, 0.0f)});
            m_currentScene->getBezierCurves()[0].nodes.push_back(BezierNode{prevNodePosition + glm::vec3(0.0f, 2.0f, 0.0f)});
        }
        for (int i = 0; i < m_currentScene->getBezierCurves()[0].nodes.size(); i++) {
            std::string label = "Control Point " + std::to_string(i);
            if (ImGui::BeginCombo(label.c_str(), "Control Point")) {
                ImGui::DragFloat3("Position", &m_currentScene->getBezierCurves()[0].nodes[i].position.x, 0.1f);
                ImGui::EndCombo();
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Scenes");
    std::string currentSceneName = "Unknown?";
    if (m_currentScene != nullptr) {
        currentSceneName = m_currentScene->getName();
    }
    ImGui::Text("Current Scene: %s", currentSceneName.c_str());
    if (ImGui::Button("SigmaVanni")) {
        m_currentScene = m_sigmaVanniScene.get();
        m_currentScene->sceneLoad();
    }
    if (ImGui::Button("Sponza")) {
        m_currentScene = m_sponzaScene.get();
        m_currentScene->sceneLoad();
    }
    if (ImGui::Button("Viking Room")) {
        m_currentScene = m_vikingRoomScene.get();
        m_currentScene->sceneLoad();
    }
    ImGui::End();

    auto& objects = m_currentScene->getGameObjects();
    int id{ 0 };
    std::function<void(Transform*)> RenderObject = [&](Transform* object) {
        ImGui::PushID(++id);
        bool treeOpen = ImGui::TreeNodeEx(std::to_string(++id).c_str(), ImGuiTreeNodeFlags_AllowOverlap);

        ImGui::SameLine();
        if (ImGui::SmallButton("Select")) {
            m_selectedTransform = object;
        }

        if (treeOpen) {
            ImGui::SeparatorText("Children");
            for (auto child : object->GetChildren()) {
                //TODO: check if i can do with VGameobjects because with just transform i don't
                //Know exactly what the owner is
                RenderObject(child);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    };

    ImGui::Begin("Scene");
    if (ImGui::Button("Deslect")) {
        m_selectedTransform = nullptr;
    }
    if (ImGui::TreeNode("ROOT")) {
            for (auto& object : objects) {
                if (!object->transform.GetParent()) { //Only do root one
                    RenderObject(&object->transform);
                }
            }
        ImGui::TreePop();
    }
    ImGui::End();

}

void VApp::loadGameObjects() {

    auto sigmaVanniSceneLoadFunction = [&](VScene* scene) {
        auto sigmaVanni = VGameObject::createGameObject();

        const auto mainSigmaVanniModel = std::make_shared<VModel>(m_device, "resources/sigmavanni/SigmaVanni.gltf", sigmaVanni.get());
        sigmaVanni->model = std::move(mainSigmaVanniModel);
        scene->addGameObject(std::move(sigmaVanni));

        auto testObject = VGameObject::createGameObject();
        const auto testModel = std::make_shared<VModel>(m_device, "resources/cat.obj", testObject.get());
        testObject->model = std::move(testModel);
        testObject->transform.SetLocalPosition({0.0f, 0.0f, 0.0f});

        scene->addGameObject(std::move(testObject));
    };

    m_sigmaVanniScene->setSceneLoadFunction(sigmaVanniSceneLoadFunction);

    auto sponzaSceneLoadFunction = [&](VScene* scene) {
        auto sponza = VGameObject::createGameObject();
        const auto mainSponzaModel = std::make_shared<VModel>(m_device, "resources/sponza/sponza.obj", sponza.get());
        sponza->model = std::move(mainSponzaModel);
        scene->addGameObject(std::move(sponza));
    };
    m_sponzaScene->setSceneLoadFunction(sponzaSceneLoadFunction);


    auto vikingRoomSceneLoadFunction = [&](VScene* scene) {
        auto vikingRoom = VGameObject::createGameObject();
        const auto mainVikingRoomModel = std::make_shared<VModel>(m_device, "resources/viking_room.obj", vikingRoom.get());
        vikingRoom->model = std::move(mainVikingRoomModel);
        scene->addGameObject(std::move(vikingRoom));
    };

    m_vikingRoomScene->setSceneLoadFunction(vikingRoomSceneLoadFunction);
}
