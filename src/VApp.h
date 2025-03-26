#ifndef VAPP_H
#define VAPP_H

#include <memory>

#include "VDevice.h"
#include "VModel.h"
#include "VPipeline.h"
#include "VSwapchain.h"
#include "VWindow.h"
#include "VGameObject.h"
#include "VRenderPass.h"
#include "Descriptors/VDescriptorPool.h"
//
//
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
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;

    VApp();
    ~VApp();

    VApp(const VApp& other) = delete;
    VApp(VApp&& other) noexcept = delete;
    VApp& operator=(const VApp& other) = delete;
    VApp& operator=(VApp&& other) noexcept = delete;

    void run();

private:

    void drawFrame();

    void recordCommandBuffer(int imageIndex);


    void loadGameObjects();

    VWindow m_window{WIDTH, HEIGHT, "Hello Vulkan!"};
    VDevice m_device{m_window};
    VRenderPass m_renderPass{m_window, m_device};

    std::vector<VGameObject> m_gameObjects;
    std::unique_ptr<VDescriptorPool> m_globalPool{};


};



#endif //VAPP_H
