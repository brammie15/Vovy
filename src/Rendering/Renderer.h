#ifndef VRENDERPASS_H
#define VRENDERPASS_H
#include <cassert>
#include <memory>

#include "Swapchain.h"
#include "Core/Device.h"
#include "Core/Window.h"

namespace vov {
    extern PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    extern PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;
}

namespace vov {
    class Renderer {
    public:
        Renderer(Window& windowRef, Device& deviceRef);
        ~Renderer();

        Renderer(const Renderer& other) = delete;
        Renderer(Renderer&& other) noexcept = delete;

        [[nodiscard]] float GetAspectRatio() const { return m_swapChain->ExtentAspectRatio(); }
        [[nodiscard]] bool IsFrameInProgress() const { return m_isFrameStarted; }

        [[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const {
            assert(m_isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[m_currentFrameIndex];
        }

        [[nodiscard]] int GetFrameIndex() const {
            assert(m_isFrameStarted && "Cannot get frame index when frame not in progress");
            return m_currentFrameIndex;
        }

        VkCommandBuffer BeginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer) const;
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer) const;

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        Window& m_window;
        Device& m_device;
        std::unique_ptr<Swapchain> m_swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t m_currentImageIndex{};
        int m_currentFrameIndex{0};
        bool m_isFrameStarted{false};
    };
}


#endif //VRENDERPASS_H
