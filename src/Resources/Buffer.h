#ifndef VBUFFER_H
#define VBUFFER_H

#include "Core/Device.h"

namespace vov {
    class Buffer {
    public:
        explicit Buffer(Device& deviceRef, VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable = false);
        ~Buffer();

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        void copyTo(const void* data, VkDeviceSize size) const;
        [[nodiscard]] VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        void flush() const;


        [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
        [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }

    private:
        Device& m_device;
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo{};

        void* m_data = nullptr;
    };
}

#endif //VBUFFER_H
