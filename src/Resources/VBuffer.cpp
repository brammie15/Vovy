#include "VBuffer.h"

VBuffer::VBuffer(VDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable): m_device{deviceRef} {
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    if (mappable) {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    } else {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    m_device.CreateBuffer(size, usageFlags, memoryUsage, mappable, m_buffer, m_allocation);
}

VBuffer::~VBuffer() {
    if (m_data != nullptr) {
        unmap();
    }
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

void VBuffer::copyTo(const void* data, VkDeviceSize size) const {
    // assert(m_data != nullptr && "Cannot copy to buffer if buffer is not mapped");
    // memcpy(m_data, data, size);
    vmaCopyMemoryToAllocation(m_device.allocator(), data, m_allocation, 0, size);
}

VkDescriptorBufferInfo VBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
    return VkDescriptorBufferInfo{
        m_buffer,
        offset,
        size
    };
}

void VBuffer::flush() const {
    vmaFlushAllocation(m_device.allocator(), m_allocation, 0, VK_WHOLE_SIZE);
}
