#ifndef VAPP_H
#define VAPP_H

#include <memory>

#include "Core/Device.h"
#include "Core/Window.h"
#include "Descriptors/DescriptorPool.h"
#include "Rendering/Pipeline.h"
#include "Rendering/Renderer.h"
#include "Rendering/Passes/BlitPass.h"
#include "Rendering/Passes/DepthPrePass.h"
#include "Rendering/Passes/GeometryPass.h"
#include "Rendering/Passes/LightingPass.h"
#include "Rendering/Passes/LinePass.h"
#include "Rendering/Passes/ShadowPass.h"
#include "Rendering/RenderSystems/ImguiRenderSystem.h"
#include "Resources/HDRI.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Utils/AppGui.h"

class VApp {
public:
    static constexpr uint32_t WIDTH = 1200;
    static constexpr uint32_t HEIGHT = 1000;

    VApp();
    ~VApp();

    VApp(const VApp& other) = delete;
    VApp(VApp&& other) noexcept = delete;
    VApp& operator=(const VApp& other) = delete;
    VApp& operator=(VApp&& other) noexcept = delete;

    void run();
    void imGui();

    void ResizeScreen(VkExtent2D newSize);

private:
    void loadGameObjects();

    double m_fpsAccumulated = 0.0;
    int m_fpsFrameCount = 0;
    double m_avgFps = 0.0;

    vov::Window m_window{WIDTH, HEIGHT, "Vovy: Verry optimal? Verry yes!"};
    vov::Device m_device{m_window};
    vov::Renderer m_renderer{m_window, m_device};

    vov::Camera m_camera{{-2.0f, 1.0f, 0}, {0.0f, 1.0f, 0.0f}};

    std::unique_ptr<vov::Scene> m_sponzaScene{};
    std::unique_ptr<vov::Scene> m_sigmaVanniScene{};
    std::unique_ptr<vov::Scene> m_vikingRoomScene{};
    std::unique_ptr<vov::Scene> m_bistroScene{};
    std::unique_ptr<vov::Scene> m_flightHelmetScene{};
    std::unique_ptr<vov::Scene> m_chessScene{};

    std::unique_ptr<vov::HDRI> m_hdrEnvironment{};

    std::vector<vov::Scene*> m_scenes{};

    vov::Transform* m_selectedTransform = nullptr;
    // vov::Transform* m_bezierFollowerTransform = nullptr;
    // float m_bezierProgress = 0.0f;
    // float m_bezierSpeed = 0.5f; // Speed at which the transform moves along the curve
    // bool m_shouldRotate = false; // Whether the transform should rotate to follow the curve

    vov::Scene* m_currentScene{nullptr};

    std::unique_ptr<vov::ImguiRenderSystem> m_imguiRenderSystem{};

    std::unique_ptr<vov::DepthPrePass> m_depthPrePass{};
    std::unique_ptr<vov::ShadowPass> m_shadowPass{};
    std::unique_ptr<vov::GeometryPass> m_geoPass{};
    std::unique_ptr<vov::LightingPass> m_lightingPass{};
    std::unique_ptr<vov::BlitPass> m_blitPass{};

    std::unique_ptr<vov::LinePass> m_linePass{};

    vov::DebugView m_currentDebugViewMode{vov::DebugView::NONE};
    bool m_showLineTools{false};
    bool m_renderImgui{ true };
    bool m_RenderBoundingBoxes{ false };

    std::unique_ptr<AppGui> m_appGui{};
};

#endif //VAPP_H
