#ifndef VMESH_H
#define VMESH_H

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <memory>
#include <glm/glm.hpp>

#include "Transform.h"
#include "Core/VDevice.h"
#include "Descriptors/VDescriptorPool.h"
#include "Descriptors/VDescriptorSetLayout.h"
#include "Resources/VBuffer.h"
#include "Resources/VImage.h"

class VMesh {
public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec2 texCoord{};
        glm::vec3 normal{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        glm::mat4 transform = glm::mat4(1.0f);
        std::string modelPath;
        std::string texturePath;

        //TODO: ask if this is properly done
        VDescriptorSetLayout* descriptorSetLayout;
        VDescriptorPool* descriptorPool;
    };

    VMesh(VDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    VMesh(VDevice& device, const Builder& builder);
    ~VMesh();

    VMesh(const VMesh&) = delete;
    VMesh& operator=(const VMesh&) = delete;
    VMesh(VMesh&&) = default;
    VMesh& operator=(VMesh&&) = default;

    [[nodiscard]] uint32_t getVertexCount() const { return m_vertexCount; }
    [[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;
    void draw(VkCommandBuffer commandBuffer) const;

    static std::unique_ptr<VMesh> createModelFromFile(
          VDevice &device, const std::string &filepath);

    Transform& getTransform() { return m_transform; }

private:
    void createVertexBuffer(const std::vector<Vertex> &vertices);
    void createIndexBuffer(const std::vector<uint32_t> &indices);
    void loadTexture(const std::string &path, VDescriptorSetLayout* descriptorSetLayout, VDescriptorPool* descriptorPool);

    VDevice &m_device;
    uint32_t m_vertexCount;
    std::unique_ptr<VBuffer> m_vertexBuffer;

    bool m_usingIndexBuffer{ false };
    uint32_t m_indexCount;
    std::unique_ptr<VBuffer> m_indexBuffer;

    VImage* m_textureImage;

    Transform m_transform;

    VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
};


#endif //VMESH_H
