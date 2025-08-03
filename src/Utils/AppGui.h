#ifndef APPGUI_H
#define APPGUI_H

#include "Scene/Scene.h"
#include "Utils/Camera.h"
#include <glm/glm.hpp>
#include <memory>

namespace vov {
    class ImguiRenderSystem;
}

class AppGui {
public:
    AppGui(vov::ImguiRenderSystem* imguiRenderSystem, vov::Scene*& scene, vov::Camera* camera);
    ~AppGui() = default;

    void Render(double avgFps, int windowWidth, int windowHeight, vov::Transform*& selectedTransform, vov::DebugView& currentDebugMode, const std::vector<vov::Scene*>& scenes);

private:
    void RenderMainMenuBar();
    void RenderSceneLight();
    void RenderStats(double avgFps, int windowWidth, int windowHeight);
    void RenderControls();
    void RenderPointLights(vov::Transform*& selectedTransform);
    void RenderCameraSettings();
    void RenderDebugModes(vov::DebugView& currentDebugMode);
    void RenderSceneSelector(const std::vector<vov::Scene*>& scenes, vov::Scene*& currentScene);


    vov::ImguiRenderSystem* m_imguiRenderSystem;
    vov::Scene*& m_scene;
    vov::Camera* m_camera;
};

#endif //APPGUI_H
