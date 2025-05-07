#ifndef VSWAPCHAIN_H
#define VSWAPCHAIN_H

#include <memory>
#include "Core/Device.h"
#include "Resources/Image.h"

namespace vov {
    class Swapchain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        explicit Swapchain(Device& deviceRef, VkExtent2D windowExtent);
        Swapchain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<Swapchain> previous);

        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        void operator=(const Swapchain&) = delete;

        [[nodiscard]] VkFormat findDepthFormat() const;
        [[nodiscard]] VkExtent2D GetSwapChainExtent() const { return m_swapChainExtent; }
        [[nodiscard]] size_t imageCount() const { return m_swapChainImages.size(); }
        [[nodiscard]] VkFormat GetSwapChainImageFormat() const { return m_swapChainImageFormat; }
        [[nodiscard]] uint32_t GetWidth() const { return m_swapChainExtent.width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_swapChainExtent.height; }
        [[nodiscard]] float ExtentAspectRatio() const;

        [[nodiscard]] VkImageView GetImageView(int index) const {
            return m_swapChainImages[index]->getImageView();
        }

        VkResult acquireNextImage(uint32_t* imageIndex) const;
        VkResult submitCommandBuffers(const VkCommandBuffer* buffers, const uint32_t* imageIndex);

        [[nodiscard]] Image& GetImage(int index) const;
        [[nodiscard]] Image& GetDepthImage(int index) const;

    private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createDepthResources();
        void createSyncObjects();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        Device& m_device;
        VkExtent2D m_windowExtent;
        VkExtent2D m_swapChainExtent{};

        VkSwapchainKHR m_swapchain{};
        VkRenderPass m_renderPass{};

        VkFormat m_swapChainImageFormat;
        VkFormat m_swapChainDepthFormat;


        //TODO: replace with vov::image
        std::vector<std::unique_ptr<vov::Image>> m_swapChainImages;
        std::vector<std::unique_ptr<vov::Image>> m_depthImages;


        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;
        size_t m_currentFrame = 0;

        std::shared_ptr<Swapchain> m_oldSwapChain;
    };
}


#endif //VSWAPCHAIN_H
