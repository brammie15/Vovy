#ifndef VSWAPCHAIN_H
#define VSWAPCHAIN_H

#include <memory>
#include "VDevice.h"


class VSwapchain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    explicit VSwapchain(VDevice& deviceRef, VkExtent2D windowExtent);
    VSwapchain(VDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<VSwapchain> previous);

    ~VSwapchain();
    [[nodiscard]] VkFramebuffer getFrameBuffer(int index) {
        return m_swapChainFramebuffers[index];
    }

    VSwapchain(const VSwapchain &) = delete;
    void operator=(const VSwapchain &) = delete;

    VkFormat findDepthFormat();
    [[nodiscard]] VkExtent2D GetSwapChainExtent() const { return m_swapChainExtent; }
    [[nodiscard]] size_t imageCount() const { return m_swapChainImages.size(); }
    [[nodiscard]] VkFormat GetSwapChainImageFormat() const { return m_swapChainImageFormat; }
    [[nodiscard]] uint32_t GetWidth() const { return m_swapChainExtent.width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_swapChainExtent.height; }

    [[nodiscard]] VkRenderPass getRenderPass() const { return m_renderPass; }

    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

private:
    void init();
    void createSwapChain();
    void createRenderPass();
    void createImageViews();
    void createDepthResources();
    void createFramebuffers();
    void createSyncObjects();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VDevice& m_device;
    VkExtent2D m_windowExtent;
    VkExtent2D m_swapChainExtent;

    VkSwapchainKHR m_swapchain;
    VkRenderPass m_renderPass;

    VkFormat m_swapChainImageFormat;
    VkFormat m_swapChainDepthFormat;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    std::vector<VkImage> m_depthImages;
    std::vector<VkDeviceMemory> m_depthImageMemorys;
    std::vector<VkImageView> m_depthImageViews;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    size_t m_currentFrame = 0;

    std::shared_ptr<VSwapchain> m_oldSwapChain;
};


#endif //VSWAPCHAIN_H
