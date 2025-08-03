#include "AppGui.h"
#include "Rendering/RenderSystems/ImguiRenderSystem.h"
#include "Scene/Lights/PointLight.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>

AppGui::AppGui(vov::ImguiRenderSystem* imguiRenderSystem, vov::Scene*& scene, vov::Camera* camera)
    : m_imguiRenderSystem(imguiRenderSystem), m_scene(scene), m_camera(camera) {}

void AppGui::Render(double avgFps, int windowWidth, int windowHeight, vov::Transform*& selectedTransform,  vov::DebugView& currentDebugMode, const std::vector<vov::Scene*>& scenes) {
    RenderMainMenuBar();
    RenderSceneLight();
    RenderStats(avgFps, windowWidth, windowHeight);
    RenderControls();
    RenderPointLights(selectedTransform);
    RenderCameraSettings();
    RenderDebugModes(currentDebugMode);
    RenderSceneSelector(scenes, m_scene);
}

void AppGui::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Save config")) {
                ImGui::SaveIniSettingsToDisk("resources/imgui.ini");
            }
            if (ImGui::MenuItem("Dump VMA")) {
                // VMA dump logic here if needed
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void AppGui::RenderSceneLight() {
    ImGui::Begin("SceneLight");

    glm::vec3& dir = m_scene->GetDirectionalLight().GetDirection();
    if (ImGui::DragFloat3("Direction", glm::value_ptr(dir), 0.01f)) {
        if (glm::length(dir) > 0.0001f)
            dir = glm::normalize(dir);
    }

    ImGui::ColorEdit3("Color", &m_scene->GetDirectionalLight().GetColor()[0]);
    ImGui::DragFloat("Intensity", &m_scene->GetDirectionalLight().GetIntensity(), 0.1f);

    ImGui::End();
}

void AppGui::RenderStats(double avgFps, int windowWidth, int windowHeight) {
    ImGui::Begin("Stats");
    ImGui::Text("Avg FPS: %.1f", avgFps);
    ImGui::Text("Window Size: %d x %d", windowWidth, windowHeight);
    ImGui::End();
}

void AppGui::RenderControls() {
    ImGui::Begin("Controls");
    ImGui::Text("WASD: Move Camera");
    ImGui::Text("Press ` to lock/unlock cursor");
    ImGui::End();
}

void AppGui::RenderPointLights(vov::Transform*& selectedTransform) {
    ImGui::Begin("Point Lights");
    if (ImGui::Button("Add Point Light")) {
        auto pointLight = std::make_unique<vov::PointLight>(glm::vec3(0.0f, 0.0f, 0.0f));
        m_scene->addPointLight(std::move(pointLight));
    }

    ImGui::Text("Point Lights: %d", m_scene->getPointLights().size());

    for (int i = 0; i < m_scene->getPointLights().size(); i++) {
        bool treeOpen = ImGui::TreeNode(("Pointlight: " + std::to_string(i)).c_str());
        ImGui::SameLine();
        ImGui::PushID(("SelectPointLight" + std::to_string(i)).c_str());
        if (ImGui::SmallButton("Select")) {
            selectedTransform = m_scene->getPointLights()[i]->GetTransform();
        }
        ImGui::PopID();
        if (treeOpen) {
            m_scene->getPointLights()[i]->RenderImGui();
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void AppGui::RenderCameraSettings() {
    ImGui::Begin("Cam Settings");

    float& iso = m_camera->GetISO();
    ImGui::DragFloat("ISO", &iso, 0.1f);

    float& aperture = m_camera->GetAperture();
    ImGui::DragFloat("Aperture", &aperture, 0.01f);

    float& shutterSpeed = m_camera->GetShutterSpeed();
    ImGui::DragFloat("Shutter Speed", &shutterSpeed, 0.001f);

    ImGui::End();


}

void AppGui::RenderDebugModes(vov::DebugView& currentDebugMode) {
    ImGui::Begin("Debug View");
    int currentIndex = static_cast<int>(currentDebugMode);

    if (ImGui::BeginCombo("Debug View", vov::DebugViewToString(currentDebugMode).c_str())) {
        for (int i = 0; i < static_cast<int>(vov::DebugView::COUNT); ++i) {
            auto view = static_cast<vov::DebugView>(i);
            bool isSelected = (i == currentIndex);
            if (ImGui::Selectable(vov::DebugViewToString(view).c_str(), isSelected)) {
                currentDebugMode = view;
                std::cout << "Selected debug view: " << vov::DebugViewToString(view) << std::endl;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}

void AppGui::RenderSceneSelector(const std::vector<vov::Scene*>& scenes, vov::Scene*& currentScene) {
    ImGui::Begin("Scene Selector");
    for (const auto & scene : scenes) {
        ImGui::PushID(scene->getName().c_str());
        if (ImGui::Button(("Load " + scene->getName()).c_str())) {
            if (currentScene != scene) {
                // currentScene->Unload();
                currentScene = scene;
                currentScene->SceneLoad();
                std::cout << "Switched to scene: " << currentScene->getName() << std::endl;
            }
        }
        ImGui::PopID();
    }
    ImGui::End();
}
