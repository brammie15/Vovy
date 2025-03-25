#ifndef VBUFFER_H
#define VBUFFER_H

#include "VDevice.h"

class VBuffer {
public:
    explicit VBuffer(VDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable = false);
    ~VBuffer();

    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();

    void copyTo(void* data, VkDeviceSize size);


    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
    [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }

private:
    VDevice& m_device;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_allocationInfo{};

    void* m_data = nullptr;

};


#endif //VBUFFER_H


