#ifndef FDEVICE_H
#define FDEVICE_H

#include <vma/vk_mem_alloc.h>

#include <vector>
#include <stdexcept>

#include "VWindow.h"

class VBuffer;

struct VulkanDeviceDebugUtilsFuncTable {
    VkDevice device;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily{};
    uint32_t presentFamily{};
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class VDevice final {
public:
    const bool enableValidationLayers = true;

    explicit VDevice(VWindow& window);
    ~VDevice();
    void copyBuffer(VBuffer* srcBuffer, VBuffer* destBuffer, uint32_t size);


    VDevice(const VDevice& other) = delete;
    VDevice(VDevice&& other) noexcept = delete;
    VDevice& operator=(const VDevice& other) = delete;
    VDevice& operator=(VDevice&& other) noexcept = delete;

    [[nodiscard]] VkCommandPool getCommandPool() const { return m_commandPool; }
    [[nodiscard]] VkDevice device() const { return m_device; }
    [[nodiscard]] VkSurfaceKHR surface() const { return m_surface; }
    [[nodiscard]] VkQueue graphicsQueue() const { return m_graphicsQueue; }
    [[nodiscard]] VkQueue presentQueue() const { return m_presentQueue; }
    [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const {return querySwapChainSupport(m_physicalDevice); }
    [[nodiscard]] VmaAllocator allocator() const { return m_allocator; }

    [[nodiscard]] VkPhysicalDeviceProperties getProperties() const { return properties; }
    QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_physicalDevice); }

    void CreateBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usageFlags,
        VmaMemoryUsage memoryUsage,
        bool mappable,

        VkBuffer& buffer,
        VmaAllocation& allocation
    );

    VkPhysicalDeviceProperties properties;

    uint32_t FindMemoryType(uint32_t typeFilter, VkFlags properties);
    void CreateImageWithInfo(
              const VkImageCreateInfo &imageInfo,
              VkMemoryPropertyFlags properties,
              VkImage &image,
              VkDeviceMemory &imageMemory);

    [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;


    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();

    void allocVmaAllocator();

    bool IsDeviceGood(VkPhysicalDevice device);
    bool CheckValidationLayerSupport() const;
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    std::vector<const char*> GetRequiredExtensions();
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;


    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VWindow &m_window;
    VkCommandPool m_commandPool;

    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    VmaAllocator m_allocator;

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};



#endif //FDEVICE_H
