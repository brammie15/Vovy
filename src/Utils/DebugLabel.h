#ifndef DEBUGLABEL_H
#define DEBUGLABEL_H



#include <mutex>
#include <string>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
#include "Core/Device.h"
#include "glm/vec4.hpp"

class DebugLabel {
public:
    static void Init(vov::Device* device);

    static bool IsAvailable();

    class ScopedCmdLabel {
    public:
        ScopedCmdLabel(VkCommandBuffer cmdBuffer, const std::string& name, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        ~ScopedCmdLabel();

        ScopedCmdLabel(const ScopedCmdLabel&) = delete;
        ScopedCmdLabel& operator=(const ScopedCmdLabel&) = delete;


    private:
        VkCommandBuffer cmdBuffer;
    };

    static void BeginCmdLabel(VkCommandBuffer cmdBuffer, const std::string& name, glm::vec4 color);

    static void EndCmdLabel(VkCommandBuffer cmdBuffer);

    static void SetObjectName(uint64_t objectHandle, VkObjectType objectType, const std::string& name);

    static void NameBuffer(VkBuffer buffer, const std::string& name);

    static void NameImage(VkImage image, const std::string& name);

    static void NameCommandBuffer(VkCommandBuffer cmdBuffer, const std::string& name);

    static void NameAllocation(VmaAllocation vma_allocation, const std::string& name);

private:
    static inline PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
    static inline PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
    static inline PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
    static inline vov::Device* s_device;
    static inline std::once_flag initFlag;
};



#endif //DEBUGLABEL_H
