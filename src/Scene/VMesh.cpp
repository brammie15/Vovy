#include "VMesh.h"

#include "Resources/VBuffer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/ext/matrix_transform.hpp>

#include "Descriptors/VDescriptorWriter.h"

std::vector<VkVertexInputBindingDescription> VMesh::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VMesh::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});

    return attributeDescriptions;
}

void VMesh::Builder::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp error: " + std::string(importer.GetErrorString()));
    }

    vertices.clear();
    indices.clear();

    for (unsigned int i = 0; i < 1; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            Vertex vertex{};

            vertex.position = {
                mesh->mVertices[j].x,
                mesh->mVertices[j].y,
                mesh->mVertices[j].z
            };

            if (mesh->HasVertexColors(0)) {
                vertex.color = {
                    mesh->mColors[0][j].r,
                    mesh->mColors[0][j].g,
                    mesh->mColors[0][j].b
                };
            } else {
                vertex.color = {1.0f, 1.0f, 1.0f}; // Default color
            }

            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord = {
                    mesh->mTextureCoords[0][j].x,
                    mesh->mTextureCoords[0][j].y
                };
            }

            vertices.push_back(vertex);
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                indices.push_back(face.mIndices[k]);
            }
        }
    }
}


VMesh::VMesh(VDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices): m_device{device} {
    createVertexBuffer(vertices);
    if (!indices.empty()) {
        createIndexBuffer(indices);
        m_usingIndexBuffer = true;
    }
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_vertexCount = static_cast<uint32_t>(vertices.size());
}

VMesh::VMesh(VDevice& device, const Builder& builder): m_device{device} {
    createVertexBuffer(builder.vertices);
    if (!builder.indices.empty()) {
        createIndexBuffer(builder.indices);
        m_usingIndexBuffer = true;
    }
    m_indexCount = static_cast<uint32_t>(builder.indices.size());
    m_vertexCount = static_cast<uint32_t>(builder.vertices.size());

    std::string texturePath = builder.modelPath + builder.texturePath;
    std::cout << "Loading texture: " << texturePath << std::endl;
    loadTexture(texturePath, builder.descriptorSetLayout, builder.descriptorPool);
}

VMesh::~VMesh() = default;

void VMesh::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const {
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

    VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (m_usingIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}

void VMesh::draw(VkCommandBuffer commandBuffer) const {
    if (m_usingIndexBuffer) {
        vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
    }
}

std::unique_ptr<VMesh> VMesh::createModelFromFile(VDevice& device, const std::string& filepath) {
    Builder builder{};
    builder.loadModel(filepath);
    return std::make_unique<VMesh>(device, builder);
}

void VMesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
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

void VMesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
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

void VMesh::loadTexture(const std::string& path, VDescriptorSetLayout* descriptorSetLayout, VDescriptorPool* descriptorPool) {
    m_textureImage = std::make_unique<VImage>(
        m_device, path, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
    auto imageInfo = m_textureImage->descriptorInfo();
    VDescriptorWriter(*descriptorSetLayout, *descriptorPool)
            .writeImage(0, &imageInfo)
            .build(m_descriptorSet);
}
