#include "VModel.h"

#include "VBuffer.h"

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

VModel::VModel(VDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices): m_device{device} {
    createVertexBuffer(vertices);
    if (!indices.empty()) {
        createIndexBuffer(indices);
        m_usingIndexBuffer = true;
    }
}

VModel::~VModel() = default;

void VModel::bind(VkCommandBuffer commandBuffer) const {
    VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (m_usingIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}

void VModel::draw(VkCommandBuffer commandBuffer) const {
    if (m_usingIndexBuffer) {
        vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
    }
}

void VModel::createVertexBuffer(const std::vector<Vertex>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());
    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    // Create staging buffer (CPU-accessible)
    std::unique_ptr<VBuffer> stagingBuffer = std::make_unique<VBuffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
    );

    stagingBuffer->map();
    stagingBuffer->copyTo((void*)vertices.data(), bufferSize);
    stagingBuffer->unmap();

    m_vertexBuffer = std::make_unique<VBuffer>(
        m_device, bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    m_device.copyBuffer(stagingBuffer.get(), m_vertexBuffer.get(), bufferSize);
}

void VModel::createIndexBuffer(const std::vector<uint32_t>& indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());
    VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    std::unique_ptr<VBuffer> stagingBuffer = std::make_unique<VBuffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
    );

    stagingBuffer->map();
    stagingBuffer->copyTo((void*)indices.data(), bufferSize);
    stagingBuffer->unmap();

    m_indexBuffer = std::make_unique<VBuffer>(
        m_device, bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    m_device.copyBuffer(stagingBuffer.get(), m_indexBuffer.get(), bufferSize);
}
