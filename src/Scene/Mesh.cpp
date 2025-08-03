#include "Mesh.h"

#include "Resources/Buffer.h"

#include <chrono>
#include <filesystem>
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
        attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)});
        attributeDescriptions.push_back({5, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, bitTangent)});

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
        m_transform.SetName(builder.name);
        m_boundingBox = builder.boundingBox;

        // std::string texturePath = builder.modelPath + builder.texturePath;
        // std::cout << "Loading texture: " << texturePath << std::endl;
        loadTexture(builder.material, builder.descriptorSetLayout, builder.descriptorPool);
    }

    void Mesh::bind(VkCommandBuffer commandBuffer) const {
        const VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
        const VkDeviceSize offsets[] = {0};
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
    }

    void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
        m_vertexCount = static_cast<uint32_t>(vertices.size());
        VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

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
        m_vertexBuffer->SetName("Vertex Buffer: " + m_transform.GetName());

        m_device.copyBuffer(stagingBuffer.get(), m_vertexBuffer.get(), bufferSize);
    }

    void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
        m_indexCount = static_cast<uint32_t>(indices.size());
        VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

        const auto stagingBuffer = std::make_unique<Buffer>(
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
        m_indexBuffer->SetName("Index Buffer: " + m_transform.GetName());

        m_device.copyBuffer(stagingBuffer.get(), m_indexBuffer.get(), bufferSize);
    }

    void Mesh::loadTexture(const Material& textureInfo, DescriptorSetLayout* descriptorSetLayout, DescriptorPool* descriptorPool) {
        m_albedoTexture =   ResourceManager::GetInstance().LoadImage(m_device, textureInfo.basePath + textureInfo.albedoPath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        m_normalTexture =   ResourceManager::GetInstance().LoadImage(m_device, textureInfo.basePath + textureInfo.normalPath, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        m_specularTexture = ResourceManager::GetInstance().LoadImage(m_device, textureInfo.basePath + textureInfo.specularPath, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        m_bumpTexture =     ResourceManager::GetInstance().LoadImage(m_device, textureInfo.basePath + textureInfo.bumpPath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        //Order is
        //Albedo
        //Normal
        //Spec
        //Bump

        const auto albedoInfo = m_albedoTexture->descriptorInfo();
        const auto normalInfo = m_normalTexture->descriptorInfo();
        const auto specularInfo = m_specularTexture->descriptorInfo();
        const auto bumpInfo = m_bumpTexture->descriptorInfo();

        auto is_file = [](const std::string& path) {
            return !path.empty();
        };

        Mesh::TextureBindingInfo info{};
        info.hasAlbedo = is_file(textureInfo.albedoPath);
        info.hasNormal = is_file(textureInfo.normalPath);
        info.hasSpecular = is_file(textureInfo.specularPath);
        info.hasBump = is_file(textureInfo.bumpPath);


        const auto stagingBuffer = std::make_unique<Buffer>(
            m_device, sizeof(Mesh::TextureBindingInfo), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
        );

        stagingBuffer->map();
        stagingBuffer->copyTo(&info, sizeof(Mesh::TextureBindingInfo));
        stagingBuffer->unmap();

        m_textureBindingInfoBuffer = std::make_unique<Buffer>(
            m_device,
            sizeof(Mesh::TextureBindingInfo),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_textureBindingInfoBuffer->SetName("Texture Binding Info Buffer: " + m_transform.GetName());

        m_device.copyBuffer(stagingBuffer.get(), m_textureBindingInfoBuffer.get(), sizeof(Mesh::TextureBindingInfo));

        const auto bufferDescription = m_textureBindingInfoBuffer->descriptorInfo();
        DescriptorWriter(*descriptorSetLayout, *descriptorPool)
                .writeBuffer(0, &bufferDescription)
                .writeImage(1, &albedoInfo)
                .writeImage(2, &normalInfo)
                .writeImage(3, &specularInfo)
                .writeImage(4, &bumpInfo)
                .build(m_descriptorSet);
    }
}
