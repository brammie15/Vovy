#include "Buffer.h"

#include "Utils/DebugLabel.h"

namespace vov {
    Buffer::Buffer(Device& deviceRef, VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable): m_device{deviceRef} {
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;

        if (mappable) {
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        } else {
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        m_size = size;
        m_device.CreateBuffer(size, usageFlags, memoryUsage, mappable, m_buffer, m_allocation);
    }

    Buffer::~Buffer() {
        if (m_data != nullptr) {
            unmap();
        }
        vmaDestroyBuffer(m_device.allocator(), m_buffer, m_allocation);
    }

    VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
        return vmaMapMemory(m_device.allocator(), m_allocation, &m_data);
    }

    void Buffer::unmap() {
        if (m_data != nullptr) {
            vmaUnmapMemory(m_device.allocator(), m_allocation);
            m_data = nullptr;
        }
    }

    void Buffer::copyTo(const void* data, VkDeviceSize size) const {
        // assert(m_data != nullptr && "Cannot copy to buffer if buffer is not mapped");
        // memcpy(m_data, data, size);
        vmaCopyMemoryToAllocation(m_device.allocator(), data, m_allocation, 0, size);
    }

    VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
        return VkDescriptorBufferInfo{
            m_buffer,
            offset,
            size
        };
    }

    void Buffer::flush() const {
        vmaFlushAllocation(m_device.allocator(), m_allocation, 0, VK_WHOLE_SIZE);
    }

    void Buffer::SetName(const std::string& name) const {
        DebugLabel::NameBuffer(m_buffer, name);
        if (m_allocation) {
            DebugLabel::NameAllocation(m_allocation, name);
        }
    }
}
