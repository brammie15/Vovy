#include "VApp.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <chrono>
#include <glm/glm.hpp>

#include <functional>
#include <iostream>
#include <thread>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Rendering/RenderSystems/GameObjectRenderSystem.h"
#include "Rendering/RenderSystems/ImguiRenderSystem.h"
#include "Rendering/RenderSystems/LineRenderSystem.h"
#include "Resources/Buffer.h"
#include "Scene/Mesh.h"
#include "Utils/BezierCurves.h"
#include "Utils/Camera.h"
#include "Utils/DeltaTime.h"
#include "Utils/FrameContext.h"

struct GlobalUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

VApp::VApp() {
    m_globalPool =
            vov::DescriptorPool::Builder(m_device)
            .setMaxSets(vov::Swapchain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vov::Swapchain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vov::Swapchain::MAX_FRAMES_IN_FLIGHT)
            .build();

    m_sigmaVanniScene = std::make_unique<vov::Scene>("SigmaVanniScene");
    m_sponzaScene = std::make_unique<vov::Scene>("SponzaScene");
    m_vikingRoomScene = std::make_unique<vov::Scene>("VikingRoomScene");
    m_bezierTestScene = std::make_unique<vov::Scene>("BezierTestScene");

    loadGameObjects();
    m_currentScene = m_vikingRoomScene.get();
    m_currentScene->sceneLoad();

    const std::vector<BezierNode> controlPoints = {
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
    std::vector<std::unique_ptr<vov::Buffer>> uboBuffers(vov::Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<vov::Buffer>(
            m_device,
            sizeof(GlobalUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = vov::DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    auto modelSetLayout = vov::DescriptorSetLayout::Builder(m_device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(vov::Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        auto success = vov::DescriptorWriter(*globalSetLayout, *m_globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);

        if (!success) {
            throw std::runtime_error("failed to write descriptor sets!");
        }
    }

    vov::GameObjectRenderSystem renderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        {globalSetLayout->getDescriptorSetLayout(), modelSetLayout->getDescriptorSetLayout()}
    };

    vov::ImguiRenderSystem imguiRenderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        static_cast<int>(m_window.getWidth()),
        static_cast<int>(m_window.getHeight())
    };

    vov::LineRenderSystem lineRenderSystem{
        m_device,
        m_renderPass.GetSwapChainRenderPass(),
        { globalSetLayout->getDescriptorSetLayout(), modelSetLayout->getDescriptorSetLayout() }
    };

    vov::Camera camera{{-2.0f, 1.0f, 0}, {0.0f, 1.0f, 0.0f}};
    camera.setAspectRatio(m_renderPass.GetAspectRatio());

    while (!m_window.ShouldClose()) {
        vov::DeltaTime::GetInstance().Update();
        m_window.PollInput();

        imguiRenderSystem.beginFrame();

        this->imGui();
        if (!m_currentScene->getGameObjects().empty() && m_selectedTransform) {
            imguiRenderSystem.drawGizmos(&camera, m_selectedTransform, "Maintransform");
        }

        if (m_bezierFollowerTransform && !m_currentScene->getBezierCurves().empty()) {
            const auto& curve = m_currentScene->getBezierCurves()[0]; // Using first curve for now
            m_bezierProgress += m_bezierSpeed * static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime());
            if (m_bezierProgress > 1.0f) m_bezierProgress = 0.0f;

            glm::vec3 position = lineRenderSystem.deCasteljau(curve.nodes, m_bezierProgress);
            m_bezierFollowerTransform->SetLocalPosition(position);

            if (m_shouldRotate) {
                float nextT = m_bezierProgress + 0.01f;
                if (nextT > 1.0f) nextT = 0.0f;
                glm::vec3 nextPosition = lineRenderSystem.deCasteljau(curve.nodes, nextT);

                glm::vec3 direction = glm::normalize(nextPosition - position);

                glm::vec3 up(0.0f, 1.0f, 0.0f); // Y is up
                glm::vec3 right = glm::normalize(glm::cross(up, direction));
                up = glm::normalize(glm::cross(direction, right));

                glm::mat4 rotationMatrix(1.0f);
                rotationMatrix[0] = glm::vec4(right, 0.0f);
                rotationMatrix[1] = glm::vec4(up, 0.0f);
                rotationMatrix[2] = glm::vec4(-direction, 0.0f); //-Z forward

                glm::quat rotation = glm::quat_cast(rotationMatrix);
                m_bezierFollowerTransform->SetLocalRotation(rotation);
            }
        }


        auto& curves = m_currentScene->getBezierCurves();
        for (auto & curve : curves) {
            for (int j = 0; j < curve.nodes.size(); j++) {
                std::string id = "BezierNode" + std::to_string(j);
                imguiRenderSystem.drawGizmos(&camera, curve.nodes[j].position, id);
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

        if (m_window.isKeyPressed(GLFW_KEY_F)) {
            if (m_selectedTransform) {
                const glm::vec3 focusPos = m_selectedTransform->GetWorldPosition();
                constexpr auto offset = glm::vec3(0.0f, 0.0f, 5.0f);  // could be camera forward
                camera.m_position = focusPos + offset;

                camera.Target(focusPos);
                camera.CalculateViewMatrix();
            }
        }

        if (m_window.isKeyPressed(GLFW_KEY_P)) {
            if (m_currentScene->getLineSegments().empty()) {
                m_currentScene->addLineSegment(vov::LineSegment{
                    {0.0f, 0.0f, 0.0f},
                    {1.0f, 1.0f, 1.0f},
                    {1.f, 1.f, 0.f}
                });
            } else {
                //Add the cameras postion to the line segments the start should be the previous line segment
                const auto& lastSegment = m_currentScene->getLineSegments().back();
                m_currentScene->addLineSegment(vov::LineSegment{
                    lastSegment.end,
                    camera.m_position,
                    {1.f, 1.f, 0.f}
                });
            }
        }

        //TODO: remove
        camera.Update(static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime()));

        camera.CalculateProjectionMatrix();
        camera.CalculateViewMatrix();

        if (auto commandBuffer = m_renderPass.BeginFrame()) {
            int frameIndex = m_renderPass.GetFrameIndex();
            vov::FrameContext frameInfo{frameIndex, static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime()), commandBuffer, globalDescriptorSets[frameIndex], camera};

            GlobalUBO ubo{};

            camera.CalculateViewMatrix();
            ubo.view = camera.GetViewMatrix();

            ubo.proj = camera.GetProjectionMatrix();

            ubo.proj[1][1] *= -1;

            uboBuffers[frameIndex]->copyTo(&ubo, sizeof(ubo));
            uboBuffers[frameIndex]->flush();

            m_renderPass.beginSwapChainRenderPass(commandBuffer);

            if (m_currentScene->getLineSegments().size() > 2) {
                // lineRenderSystem.renderLines(frameInfo, m_currentScene->getLineSegments());
            }

            lineRenderSystem.renderBezier(frameInfo, m_currentScene->getBezierCurves());

            renderSystem.renderGameObjects(frameInfo, m_currentScene->getGameObjects());

            imguiRenderSystem.renderImgui(commandBuffer);
            m_renderPass.endSwapChainRenderPass(commandBuffer);
            m_renderPass.endFrame();
        }

        // FPS limiter
        const auto sleepTime = vov::DeltaTime::GetInstance().SleepDuration();
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
    ImGui::Text("FPS: %.1f", 1.0f / vov::DeltaTime::GetInstance().GetDeltaTime());
    ImGui::Text("Frame Time: %.3f ms", vov::DeltaTime::GetInstance().GetDeltaTime() * 1000.0f);
    ImGui::Text("Window Size: %d x %d", m_window.getWidth(), m_window.getHeight());
    ImGui::End();

    ImGui::Begin("Controls");
    ImGui::Text("Controls:");
    ImGui::Text("WASD: Move Camera");
    ImGui::Text("Press ` to lock/unlock cursor");
    ImGui::Text("Press F to focus on selected object (BIG WIP)");
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
    ImGui::Text("Bezier Segments: %d", m_currentScene->getBezierCurves().size());

    static char filenameBuffer[256] = "curves.bez"; // Default filename

    ImGui::Separator();
    ImGui::InputText("File", filenameBuffer, IM_ARRAYSIZE(filenameBuffer));

    if (ImGui::Button("Save Curves")) {
        BezierSerializer::writeBezierCurves(filenameBuffer, m_currentScene->getBezierCurves());
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Curves")) {
        const auto loadedCurves = BezierSerializer::readBezierCurves(filenameBuffer);
        m_currentScene->getBezierCurves() = loadedCurves;
    }

    auto& curves = m_currentScene->getBezierCurves();
    if (ImGui::Button("Add Curve")) {
        curves.push_back({
            {},
            glm::vec3(1.0f, 0.0f, 0.0f),
            100
        });
    }

    for (size_t curveIndex = 0; curveIndex < curves.size(); ++curveIndex) {
        std::string curveLabel = "Curve " + std::to_string(curveIndex);
        if (ImGui::TreeNode(curveLabel.c_str())) {
            auto& curve = curves[curveIndex];

            ImGui::Text("Nodes: %zu", curve.nodes.size());

            std::string addNodeLabel = "Add Node##" + std::to_string(curveIndex);
            if (ImGui::Button(addNodeLabel.c_str())) {
                glm::vec3 newPos(0.0f, 0.0f, 0.0f);
                if (!curve.nodes.empty()) {
                    newPos = curve.nodes.back().position + glm::vec3(0.0f, 1.0f, 0.0f);
                }
                curve.nodes.emplace_back(newPos);
            }
            for (size_t nodeIndex = 0; nodeIndex < curve.nodes.size(); ++nodeIndex) {
                std::string nodeLabel = "Node " + std::to_string(nodeIndex);
                if (ImGui::TreeNode(nodeLabel.c_str())) {
                    ImGui::DragFloat3("Position", &curve.nodes[nodeIndex].position.x, 0.1f);
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::End();

    ImGui::Begin("Model Loader");

    static char modelPath[256] = "models/viking_room.obj"; // default path

    ImGui::InputText("Model Path", modelPath, IM_ARRAYSIZE(modelPath));

    if (ImGui::Button("Load Model")) {
        std::string pathStr = modelPath;
        auto modelGameObject = vov::GameObject::LoadModelFromDisk(m_device, pathStr);
        if (modelGameObject) {
            std::cout << "Loaded model from: " << pathStr << std::endl;
            m_currentScene->addGameObject(std::move(modelGameObject));
        } else {
            std::cerr << "Failed to load model from: " << pathStr << std::endl;
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
    if (ImGui::Button("Bezier Test")) {
        m_currentScene = m_bezierTestScene.get();
        m_currentScene->sceneLoad();
    }
    ImGui::End();

    const auto& objects = m_currentScene->getGameObjects();
    int id{ 0 };
    std::function<void(vov::Transform*)> RenderObject = [&](vov::Transform* object) {
        ImGui::PushID(++id);
        const bool treeOpen = ImGui::TreeNodeEx(std::to_string(++id).c_str(), ImGuiTreeNodeFlags_AllowOverlap);

        ImGui::SameLine();
        if (ImGui::SmallButton("Select")) {
            m_selectedTransform = object;
        }

        if (treeOpen) {
            ImGui::SeparatorText("Children");
            for (const auto child : object->GetChildren()) {
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

    ImGui::Begin("Bezier Curve Follower");
    if (ImGui::Button("Set Selected as Follower")) {
        m_bezierFollowerTransform = m_selectedTransform;
    }
    if (ImGui::Button("Clear Follower")) {
        m_bezierFollowerTransform = nullptr;
    }
    if (m_bezierFollowerTransform) {
        ImGui::Text("Current Follower: %p", m_bezierFollowerTransform);
        ImGui::SliderFloat("Follow Speed", &m_bezierSpeed, 0.1f, 2.0f);
        ImGui::SliderFloat("Progress", &m_bezierProgress, 0.0f, 1.0f);
        ImGui::Checkbox("Should Rotate", &m_shouldRotate);
    }
    ImGui::End();
}

void VApp::loadGameObjects() {

    auto sigmaVanniSceneLoadFunction = [&](vov::Scene* scene) {
        auto sigmaVanni = vov::GameObject::createGameObject();

        const auto mainSigmaVanniModel = std::make_shared<vov::Model>(m_device, "resources/sigmavanni/SigmaVanni.gltf", sigmaVanni.get());
        sigmaVanni->model = std::move(mainSigmaVanniModel);
        scene->addGameObject(std::move(sigmaVanni));

        auto testObject = vov::GameObject::createGameObject();
        const auto testModel = std::make_shared<vov::Model>(m_device, "resources/cat.obj", testObject.get());
        testObject->model = std::move(testModel);
        testObject->transform.SetLocalPosition({0.0f, 0.0f, 0.0f});

        scene->addGameObject(std::move(testObject));
    };

    m_sigmaVanniScene->setSceneLoadFunction(sigmaVanniSceneLoadFunction);

    auto sponzaSceneLoadFunction = [&](vov::Scene* scene) {
        auto sponza = vov::GameObject::createGameObject();
        const auto mainSponzaModel = std::make_shared<vov::Model>(m_device, "resources/sponza/sponza.obj", sponza.get());
        sponza->model = std::move(mainSponzaModel);
        scene->addGameObject(std::move(sponza));
    };
    m_sponzaScene->setSceneLoadFunction(sponzaSceneLoadFunction);


    auto vikingRoomSceneLoadFunction = [&](vov::Scene* scene) {
        auto vikingRoom = vov::GameObject::createGameObject();
        const auto mainVikingRoomModel = std::make_shared<vov::Model>(m_device, "resources/viking_room.obj", vikingRoom.get());
        vikingRoom->model = std::move(mainVikingRoomModel);
        scene->addGameObject(std::move(vikingRoom));
    };

    m_vikingRoomScene->setSceneLoadFunction(vikingRoomSceneLoadFunction);

    auto bezierTestSceneLoadFunction = [&](vov::Scene* scene) {
        auto bezierTestObject = vov::GameObject::createGameObject();
        const auto bezierTestModel = std::make_shared<vov::Model>(m_device, "resources/XYZaxis.obj", bezierTestObject.get());
        bezierTestObject->model = std::move(bezierTestModel);
        scene->addGameObject(std::move(bezierTestObject));
    };

    m_bezierTestScene->setSceneLoadFunction(bezierTestSceneLoadFunction);
}
