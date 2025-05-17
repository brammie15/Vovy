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

#include "Descriptors/DescriptorWriter.h"
#include "Rendering/RenderSystems/ImguiRenderSystem.h"
#include "Rendering/RenderSystems/LineRenderSystem.h"
#include "Resources/Buffer.h"
#include "Scene/DirectionalLight.h"
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
            .setMaxSets(vov::Swapchain::MAX_FRAMES_IN_FLIGHT * 3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vov::Swapchain::MAX_FRAMES_IN_FLIGHT * 3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vov::Swapchain::MAX_FRAMES_IN_FLIGHT * 3)
            .build();

    m_sigmaVanniScene = std::make_unique<vov::Scene>("SigmaVanniScene");
    m_sponzaScene = std::make_unique<vov::Scene>("SponzaScene");
    m_vikingRoomScene = std::make_unique<vov::Scene>("VikingRoomScene");
    m_sibenikScene = std::make_unique<vov::Scene>("SibenikScene");
    m_flightHelmetScene = std::make_unique<vov::Scene>("FligtHelmetScene");

    m_scenes.emplace_back(m_sigmaVanniScene.get());
    m_scenes.emplace_back(m_sponzaScene.get());
    m_scenes.emplace_back(m_vikingRoomScene.get());
    m_scenes.emplace_back(m_sibenikScene.get());
    m_scenes.emplace_back(m_flightHelmetScene.get());


    loadGameObjects();
    m_currentScene = m_flightHelmetScene.get();
    m_currentScene->SceneLoad();

    m_renderer.SetResizeCallback([&] (const VkExtent2D newSize) {
        this->ResizeScreen(newSize);
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

    m_depthPrePass = std::make_unique<vov::DepthPrePass>(
        m_device
    );
    m_depthPrePass->Init(VK_FORMAT_D32_SFLOAT, vov::Swapchain::MAX_FRAMES_IN_FLIGHT);

    vov::GeometryPass::CreateInfo createInfo = {
        vov::Swapchain::MAX_FRAMES_IN_FLIGHT, m_currentScene, m_window.getExtent(), VK_FORMAT_D32_SFLOAT
    };

    m_geoPass = std::make_unique<vov::GeometryPass>(m_device, createInfo);

    //TODO: figure out format here;
    m_lightingPass = std::make_unique<vov::LightingPass>(m_device, vov::Swapchain::MAX_FRAMES_IN_FLIGHT, VK_FORMAT_R16G16B16A16_SFLOAT, m_renderer.getSwapchain().GetSwapChainExtent());

    m_blitPass = std::make_unique<vov::BlitPass>(
        m_device, vov::Swapchain::MAX_FRAMES_IN_FLIGHT, *m_lightingPass, m_renderer.getSwapchain()
    );

    vov::ImguiRenderSystem imguiRenderSystem{
        m_device,
        VK_FORMAT_B8G8R8A8_SRGB, //TODO: this should really be a global
        static_cast<int>(m_window.getWidth()),
        static_cast<int>(m_window.getHeight())
    };

    m_camera.setAspectRatio(m_renderer.GetAspectRatio());

    while (!m_window.ShouldClose()) {
        vov::DeltaTime::GetInstance().Update();
        const double currentFps = 1.0 / vov::DeltaTime::GetInstance().GetDeltaTime();
        m_fpsAccumulated += currentFps;
        m_fpsFrameCount++;

        if (m_fpsFrameCount >= 100) {
            // Average over 100 frames
            m_avgFps = m_fpsAccumulated / m_fpsFrameCount;
            m_fpsAccumulated = 0.0;
            m_fpsFrameCount = 0;
        }

#pragma region INPUT
        m_window.PollInput();
        imguiRenderSystem.beginFrame();

        this->imGui();
        if (!m_currentScene->getGameObjects().empty() && m_selectedTransform) {
            imguiRenderSystem.drawGizmos(&m_camera, m_selectedTransform, "Maintransform");
        }
        imguiRenderSystem.endFrame();

        if (m_window.isKeyPressed(GLFW_KEY_GRAVE_ACCENT)) {
            if (m_window.isCursorLocked()) {
                m_window.UnlockCursor();
            } else {
                m_window.LockCursor();
            }
        }
#pragma endregion

        m_camera.Update(static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime()));

        m_camera.CalculateProjectionMatrix();
        m_camera.CalculateViewMatrix();

        if (const auto commandBuffer = m_renderer.BeginFrame()) {
            const int frameIndex = m_renderer.GetFrameIndex();
            vov::FrameContext frameContext{frameIndex, static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime()), commandBuffer, m_camera};

            auto& depthImage = m_renderer.GetCurrentDepthImage();
            m_depthPrePass->Record(frameContext, commandBuffer, frameIndex, depthImage, m_currentScene, &m_camera);

            m_geoPass->Record(frameContext, commandBuffer, frameIndex, depthImage, m_currentScene, &m_camera);

            m_lightingPass->UpdateDescriptors(
                frameIndex, m_geoPass->GetAlbedo(frameIndex), m_geoPass->GetNormal(frameIndex), m_geoPass->GetSpecualar(frameIndex), m_geoPass->GetWorldPos(frameIndex)
            );

            m_lightingPass->Record(frameContext, commandBuffer, frameIndex, *m_geoPass, *m_currentScene);

            m_renderer.beginSwapChainRenderPass(commandBuffer);

            // if (m_currentScene->getLineSegments().size() > 2) {
            //     lineRenderSystem.renderLines(frameInfo, m_currentScene->getLineSegments());
            // }

            // lineRenderSystem.renderBezier(frameInfo, m_currentScene->getBezierCurves());
            // renderSystem.renderGameObjects(frameInfo, m_currentScene->getGameObjects());

            m_blitPass->Record(frameContext, commandBuffer, frameIndex, m_renderer.getSwapchain());
            imguiRenderSystem.renderImgui(commandBuffer);


            m_renderer.endSwapChainRenderPass(commandBuffer);
            m_renderer.endFrame();
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
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Save config")) {
                ImGui::SaveIniSettingsToDisk("resources/imgui.ini");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }


    ImGui::Begin("SceneLight");
    ImGui::Text("Light Direction: ");
    // if (ImGui::DragFloat3("Light Direction", glm::value_ptr(m_currentScene->GetDirectionalLight().GetDirection()), 0.1f)) {
    //     m_currentScene->GetDirectionalLight().setDirection(m_currentScene->GetDirectionalLight().GetDirection());
    // }
    glm::vec3& dir = m_currentScene->GetDirectionalLight().GetDirection();
    if (ImGui::DragFloat3("##Direction", glm::value_ptr(dir), 0.01f)) {
        if (glm::length(dir) > 0.0001f)
            dir = glm::normalize(dir);
    }
    ImGui::Text("Light Color: ");
    ImGui::ColorEdit3("Light Color", &m_currentScene->GetDirectionalLight().GetColor()[0]);
    ImGui::Text("Light Intensity: ");
    ImGui::DragFloat("Light Intensity", &m_currentScene->GetDirectionalLight().GetIntensity(), 0.1f);

    ImGui::End();

    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", 1.0f / vov::DeltaTime::GetInstance().GetDeltaTime());
    ImGui::Text("Avg FPS: %.1f", m_avgFps);
    ImGui::Text("Frame Time: %.3f ms", vov::DeltaTime::GetInstance().GetDeltaTime() * 1000.0f);
    ImGui::Text("Window Size: %d x %d", m_window.getWidth(), m_window.getHeight());
    ImGui::End();

    ImGui::Begin("Controls");
    ImGui::Text("Controls:");
    ImGui::Text("WASD: Move Camera");
    ImGui::Text("Press ` to lock/unlock cursor");
    ImGui::Text("Press F to focus on selected object (BIG WIP)");
    ImGui::End();

    ImGui::Begin("Cam Settings");

    float& exp = m_camera.GetExposure();
    ImGui::DragFloat("Exposure", &exp, 0.01f, 0);


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
    if(ImGui::Button("Purge Scenes")) {
        std::ranges::for_each(m_scenes, [&](vov::Scene* scene) {
            if (scene != m_currentScene) {
                scene->SceneUnLoad();
            }
        });
    }
    //TODO: replace with the vector
    if (ImGui::Button("SigmaVanni")) {
        m_currentScene = m_sigmaVanniScene.get();
        m_currentScene->SceneLoad();
    }
    if (ImGui::Button("Sponza")) {
        m_currentScene = m_sponzaScene.get();
        m_currentScene->SceneLoad();
    }
    if (ImGui::Button("Viking Room")) {
        m_currentScene = m_vikingRoomScene.get();
        m_currentScene->SceneLoad();
    }
    if (ImGui::Button("Flight Helmet")) {
        m_currentScene = m_flightHelmetScene.get();
        m_currentScene->SceneLoad();
    }
    if (ImGui::Button("Sibenik")) {
        m_currentScene = m_sibenikScene.get();
        m_currentScene->SceneLoad();
    }
    ImGui::End();

    const auto& objects = m_currentScene->getGameObjects();
    int id{0};
    std::function<void(vov::Transform*)> RenderObject = [&] (vov::Transform* object) {
        ImGui::PushID(++id);
        std::string name = object->GetName();
        const bool treeOpen = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_AllowOverlap);

        ImGui::SameLine();
        if (ImGui::SmallButton("Select")) {
            m_selectedTransform = object;
        }

        if (treeOpen) {
            ImGui::SeparatorText("Children");
            for (const auto child: object->GetChildren()) {
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
        for (auto& object: objects) {
            if (!object->transform.GetParent()) {
                //Only do root one
                RenderObject(&object->transform);
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();

    if (m_showLineTools) {

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

        static char filenameBuffer[256] = "resources/curves.bram"; // Default filename

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
}

void VApp::ResizeScreen(const VkExtent2D newSize) {
    m_depthPrePass->Resize(newSize);
    m_geoPass->Resize(newSize);
    m_lightingPass->Resize(newSize);
    m_blitPass->Resize(newSize, *m_lightingPass);

    m_camera.setAspectRatio(static_cast<float>(newSize.width) / static_cast<float>(newSize.height));
}

void VApp::loadGameObjects() {
    m_sigmaVanniScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto sigmaVanni = vov::GameObject::LoadModelFromDisk(m_device, "resources/sigmavanni/SigmaVanni.gltf");
        scene->addGameObject(std::move(sigmaVanni));

        auto& directionalLight = scene->GetDirectionalLight();
        // directionalLight.setDirection(glm::vec3(0.0f, -0.5f, 0.5f));
        directionalLight.setColor(glm::vec3(1.0f, 1.0f, 1.0f));
        directionalLight.setIntensity(1.0f);
    });

    m_sponzaScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto sponza = vov::GameObject::createGameObject();
       const auto mainSponzaModel = std::make_shared<vov::Model>(m_device, "resources/Sponza/Sponza.gltf", sponza.get());
       sponza->model = std::move(mainSponzaModel);
       scene->addGameObject(std::move(sponza));
    });

    m_vikingRoomScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto vikingRoom = vov::GameObject::LoadModelFromDisk(m_device, "resources/viking_room.obj");
        scene->addGameObject(std::move(vikingRoom));
    });

    m_sibenikScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto sibenik = vov::GameObject::LoadModelFromDisk(m_device, "resources/sibenik/sibenik.obj");
        scene->addGameObject(std::move(sibenik));
    });

    m_flightHelmetScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto flightHelmet = vov::GameObject::LoadModelFromDisk(m_device, "resources/FlightHelmet/FlightHelmet.gltf");
        scene->addGameObject(std::move(flightHelmet));
    });
}
