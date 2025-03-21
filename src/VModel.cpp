#include "VModel.h"

std::vector<VkVertexInputBindingDescription> VModel::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VModel::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});

    return attributeDescriptions;
}

VModel::VModel(VDevice& device, const std::vector<Vertex>& vertices): m_device{device} {
    createVertexBuffer(vertices);
}

VModel::~VModel() {
    vkDestroyBuffer(m_device.device(), m_vertexBuffer, nullptr);
    vmaFreeMemory(m_device.allocator(), m_vertexAllocation);
}

void VModel::bind(VkCommandBuffer commandBuffer) const {
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

void VModel::draw(VkCommandBuffer commandBuffer) const {
    vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
}

void VModel::createVertexBuffer(const std::vector<Vertex>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());

    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    m_device.CreateBuffer(bufferSize, bufferUsage, memoryUsage, true, m_vertexBuffer, m_vertexAllocation);



    void* data;
    vmaMapMemory(m_device.allocator(), m_vertexAllocation, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(m_device.allocator(), m_vertexAllocation);

    //TODO: look into vma more cuz damm
}
