#include "Swapchain.h"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace vov {
    Swapchain::Swapchain(Device& deviceRef, VkExtent2D windowExtent): m_device(deviceRef), m_windowExtent{windowExtent} {
        createSwapChain();
        createDepthResources();
        createSyncObjects();
    }

    Swapchain::Swapchain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<Swapchain> previous): m_device{deviceRef}, m_windowExtent{windowExtent}, m_oldSwapChain{std::move(previous)} {
        init();
        m_oldSwapChain = nullptr;
    }

    Swapchain::~Swapchain() {
        m_swapChainImages.clear();
        m_depthImages.clear();

        for (auto semaphore: m_imageAvailableSemaphores) {
            vkDestroySemaphore(m_device.device(), semaphore, nullptr);
        }
        for (auto semaphore: m_renderFinishedSemaphores) {
            vkDestroySemaphore(m_device.device(), semaphore, nullptr);
        }
        for (auto fence: m_inFlightFences) {
            vkDestroyFence(m_device.device(), fence, nullptr);
        }

        if (m_swapchain != nullptr) {
            vkDestroySwapchainKHR(m_device.device(), m_swapchain, nullptr);
            m_swapchain = nullptr;
        }

        vkDestroyRenderPass(m_device.device(), m_renderPass, nullptr);
    }

    VkFormat Swapchain::findDepthFormat() const {
        return m_device.FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    float Swapchain::ExtentAspectRatio() const {
        return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height);
    }

    Image& Swapchain::GetImage(int index) const {
        assert(index < m_swapChainImages.size() && "Current frame index out of range");
        return *m_swapChainImages[index];
    }

    Image& Swapchain::GetDepthImage(int index) const {
        assert(index < m_depthImages.size() && "Current frame index out of range");
        return *m_depthImages[index];
    }

    VkResult Swapchain::acquireNextImage(uint32_t* imageIndex) const {
        vkWaitForFences(
            m_device.device(),
            1,
            &m_inFlightFences[m_currentFrame],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());

        const auto result = vkAcquireNextImageKHR(
            m_device.device(),
            m_swapchain,
            std::numeric_limits<uint64_t>::max(),
            m_imageAvailableSemaphores[m_currentFrame], // ✅ correct
            VK_NULL_HANDLE,
            imageIndex);

        return result;
    }

    VkResult Swapchain::submitCommandBuffers(const VkCommandBuffer* buffers, const uint32_t* imageIndex) {
        if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(m_device.device(), 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
        constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;

        const VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(m_device.device(), 1, &m_inFlightFences[m_currentFrame]);
        if (vkQueueSubmit(m_device.graphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;

        const VkSwapchainKHR swapChains[] = {m_swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndex;
        presentInfo.pWaitSemaphores = signalSemaphores;

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return vkQueuePresentKHR(m_device.presentQueue(), &presentInfo);
    }

    void Swapchain::init() {
        createSwapChain();
        createDepthResources();
        createSyncObjects();
    }

    void Swapchain::createSwapChain() {
        const SwapChainSupportDetails swapChainSupport = m_device.getSwapChainSupport();

        const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_device.surface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const QueueFamilyIndices indices = m_device.FindPhysicalQueueFamilies();
        const uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = m_oldSwapChain ? m_oldSwapChain->m_swapchain : VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device.device(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device.device(), m_swapchain, &imageCount, nullptr);
        std::vector<VkImage> vkImages(imageCount);
        vkGetSwapchainImagesKHR(m_device.device(), m_swapchain, &imageCount, vkImages.data());

        m_swapChainImages.clear();
        for (auto image: vkImages) {
            auto vovImage = std::make_unique<vov::Image>(
                m_device,
                extent,
                surfaceFormat.format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY,
                image);
            m_swapChainImages.push_back(std::move(vovImage));
        }

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void Swapchain::createDepthResources() {
        m_depthImages.clear();
        m_swapChainDepthFormat = findDepthFormat();

        for (size_t i = 0; i < imageCount(); i++) {
            auto depthImage = std::make_unique<vov::Image>(
                m_device,
                m_swapChainExtent,
                m_swapChainDepthFormat,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            m_depthImages.push_back(std::move(depthImage));
        }
    }

    void Swapchain::createSyncObjects() {
        size_t imageCount = this->imageCount();

        m_imageAvailableSemaphores.resize(imageCount);
        m_renderFinishedSemaphores.resize(imageCount);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < imageCount; i++) {
            if (vkCreateSemaphore(m_device.device(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device.device(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization semaphores for swapchain images!");
            }
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateFence(m_device.device(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create frame fences!");
            }
        }
    }

    VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = m_windowExtent;
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }
}
