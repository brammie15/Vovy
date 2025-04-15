#ifndef VAPP_H
#define VAPP_H

#include <memory>

#include "Core/VDevice.h"
#include "Core/VWindow.h"
#include "Descriptors/VDescriptorPool.h"
#include "Rendering/VPipeline.h"
#include "Rendering/VRenderPass.h"
#include "Scene/VGameObject.h"
#include "Scene/VScene.h"

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

    VWindow m_window{WIDTH, HEIGHT, "Hello Vulkan!"};
    VDevice m_device{m_window};
    VRenderPass m_renderPass{m_window, m_device};

    std::unique_ptr<VScene> m_sigmaVanniScene{};
    std::unique_ptr<VScene> m_sponzaScene{};
    std::unique_ptr<VScene> m_vikingRoomScene{};

    Transform* m_selectedTransform{ nullptr };

    VScene* m_currentScene{ nullptr };
    std::unique_ptr<VDescriptorPool> m_globalPool{};
};

#endif //VAPP_H
