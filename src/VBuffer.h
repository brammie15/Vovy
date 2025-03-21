#ifndef VBUFFER_H
#define VBUFFER_H

#include "VDevice.h"

class VBuffer {
public:
    explicit VBuffer(VDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    //
    // VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    // void unmap();

    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }

private:
    VDevice& m_device;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
};


#endif //VBUFFER_H
