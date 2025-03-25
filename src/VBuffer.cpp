#include "VBuffer.h"

#include <cassert>
#include <cstring>

VBuffer::VBuffer(VDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable): m_device{deviceRef} {
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    if (mappable) {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    } else {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    m_device.CreateBuffer(size, usageFlags, memoryUsage, mappable, m_buffer, m_allocation);
}

VBuffer::~VBuffer() {
    vmaDestroyBuffer(m_device.allocator(), m_buffer, m_allocation);
}

VkResult VBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
    return vmaMapMemory(m_device.allocator(), m_allocation, &m_data);
}

void VBuffer::unmap() {
    if (m_data != nullptr) {
        vmaUnmapMemory(m_device.allocator(), m_allocation);
        m_data = nullptr;
    }
}

void VBuffer::copyTo(void* data, VkDeviceSize size) {
    assert(m_data != nullptr && "Cannot copy to buffer if buffer is not mapped");
    memcpy(m_data, data, size);
}
