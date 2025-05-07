#ifndef VAPP_H
#define VAPP_H

#include <memory>

#include "Core/Device.h"
#include "Core/Window.h"
#include "Descriptors/DescriptorPool.h"
#include "Rendering/Pipeline.h"
#include "Rendering/Renderer.h"
#include "Rendering/Passes/DepthPrePass.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

// namespace std {
//     template<typename T>
//     using freaky_ptr = unique_ptr<T>;
//
//     template<typename T>
//     using thalia_ptr = shared_ptr<T>;
//
//     template <typename T, typename... Args>
//     freaky_ptr<T> get_freaky(Args&&... args) {
//         return std::make_unique<T>(std::forward<Args>(args)...);
//     }
// }

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

private:
    void loadGameObjects();

    vov::Window m_window{WIDTH, HEIGHT, "Hello Vulkan!"};
    vov::Device m_device{m_window};
    vov::Renderer m_renderer{m_window, m_device};

    std::unique_ptr<vov::Scene> m_sigmaVanniScene{};
    std::unique_ptr<vov::Scene> m_sponzaScene{};
    std::unique_ptr<vov::Scene> m_vikingRoomScene{};
    std::unique_ptr<vov::Scene> m_bezierTestScene{};

    vov::Transform* m_selectedTransform = nullptr;
    vov::Transform* m_bezierFollowerTransform = nullptr;
    float m_bezierProgress = 0.0f;
    float m_bezierSpeed = 0.5f; // Speed at which the transform moves along the curve
    bool m_shouldRotate = false; // Whether the transform should rotate to follow the curve

    vov::Scene* m_currentScene{ nullptr };
    std::unique_ptr<vov::DescriptorPool> m_globalPool{};

    std::unique_ptr<vov::DepthPrePass> m_depthPrePass{};

};

#endif //VAPP_H
