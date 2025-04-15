#ifndef VRENDERPASS_H
#define VRENDERPASS_H
#include <cassert>
#include <memory>

#include "VSwapchain.h"
#include "Core/VDevice.h"
#include "Core/VWindow.h"


class VRenderPass {
public:
    VRenderPass(VWindow& windowRef, VDevice& deviceRef);
    ~VRenderPass();

    VRenderPass(const VRenderPass& other) = delete;
    VRenderPass(VRenderPass&& other) noexcept = delete;

    [[nodiscard]] VkRenderPass GetSwapChainRenderPass() const { return m_swapChain->GetRenderPass(); }
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

    VWindow& m_window;
    VDevice& m_device;
    std::unique_ptr<VSwapchain> m_swapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t m_currentImageIndex{};
    int m_currentFrameIndex{0};
    bool m_isFrameStarted{false};
};


#endif //VRENDERPASS_H
