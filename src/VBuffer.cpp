#include "VBuffer.h"

#include <cassert>

VBuffer::VBuffer(VDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage): m_device{deviceRef} {

}
//
// VkResult VBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
//   assert(m_buffer && m_memory && "Can't map a not yet created buffer?");
//   return vkMapMemory(m_device.device(), m_memory, offset, size, 0, &m_mapped);
// }
//
// void VBuffer::unmap() {
//   if (m_mapped) {
//     vkUnmapMemory(m_device.device(), m_memory);
//     m_mapped = nullptr;
//   }
// }

