#include "Mesh.h"

#include "Resources/Buffer.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

#include <assimp/scene.h>

#include "Descriptors/DescriptorWriter.h"
#include "Utils/ResourceManager.h"

namespace vov {
    std::vector<VkVertexInputBindingDescription> Mesh::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Mesh::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});

        return attributeDescriptions;
    }

    Mesh::Mesh(Device& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices): m_device{device} {
        createVertexBuffer(vertices);
        if (!indices.empty()) {
            createIndexBuffer(indices);
            m_usingIndexBuffer = true;
        }
        m_indexCount = static_cast<uint32_t>(indices.size());
        m_vertexCount = static_cast<uint32_t>(vertices.size());
    }

    Mesh::Mesh(Device& device, const Builder& builder): m_device{device} {
        createVertexBuffer(builder.vertices);
        if (!builder.indices.empty()) {
            createIndexBuffer(builder.indices);
            m_usingIndexBuffer = true;
        }
        m_indexCount = static_cast<uint32_t>(builder.indices.size());
        m_vertexCount = static_cast<uint32_t>(builder.vertices.size());

        const std::string texturePath = builder.modelPath + builder.texturePath;
        std::cout << "Loading texture: " << texturePath << std::endl;
        loadTexture(texturePath, builder.descriptorSetLayout, builder.descriptorPool);
    }

    Mesh::~Mesh() = default;

    void Mesh::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            1,
            1,
            &m_descriptorSet,
            0,
            nullptr
        );

        const VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
        constexpr VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (m_usingIndexBuffer) {
            vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void Mesh::draw(VkCommandBuffer commandBuffer) const {
        if (m_usingIndexBuffer) {
            vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
        } else {
            vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
        }
    }

    std::unique_ptr<Mesh> Mesh::createModelFromFile(Device& device, const std::string& filepath) {
        Builder builder{};
        throw std::runtime_error("Not implemented yet");
        // return std::make_unique<VMesh>(device, builder);
    }

    void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
        m_vertexCount = static_cast<uint32_t>(vertices.size());
        VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

        // Create staging buffer (CPU-accessible)
        const auto stagingBuffer = std::make_unique<Buffer>(
            m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
        );

        stagingBuffer->map();
        stagingBuffer->copyTo((void*)vertices.data(), bufferSize);
        stagingBuffer->unmap();

        m_vertexBuffer = std::make_unique<Buffer>(
            m_device, bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );

        m_device.copyBuffer(stagingBuffer.get(), m_vertexBuffer.get(), bufferSize);
    }

    void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
        m_indexCount = static_cast<uint32_t>(indices.size());
        const VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

        auto stagingBuffer = std::make_unique<Buffer>(
            m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
        );

        stagingBuffer->map();
        stagingBuffer->copyTo((void*)indices.data(), bufferSize);
        stagingBuffer->unmap();

        m_indexBuffer = std::make_unique<Buffer>(
            m_device, bufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );

        m_device.copyBuffer(stagingBuffer.get(), m_indexBuffer.get(), bufferSize);
    }

    void Mesh::loadTexture(const std::string& path, DescriptorSetLayout* descriptorSetLayout, DescriptorPool* descriptorPool) {
        m_textureImage = ResourceManager::GetInstance().loadImage(m_device, path, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        const auto imageInfo = m_textureImage->descriptorInfo();
        DescriptorWriter(*descriptorSetLayout, *descriptorPool)
                .writeImage(0, &imageInfo)
                .build(m_descriptorSet);
    }
}
