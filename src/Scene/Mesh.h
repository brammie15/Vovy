#ifndef VMESH_H
#define VMESH_H

#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Transform.h"
#include "Core/Device.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Resources/Buffer.h"
#include "Resources/Image.h"
#include "Utils/AABB.h"

namespace vov {
    class Mesh {
    public:
        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec2 texCoord{};
            glm::vec3 normal{};
            glm::vec3 tangent{};
            glm::vec3 bitTangent{};

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        struct Material {
            std::string basePath;

            std::string albedoPath;
            std::string normalPath;
            std::string bumpPath;
            std::string specularPath;
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
            glm::mat4 transform = glm::mat4(1.0f);
            std::string modelPath{};
            std::string name{};

            Material material{};
            AABB boundingBox{};

            //TODO: ask if this is properly done
            DescriptorSetLayout* descriptorSetLayout{};
            DescriptorPool* descriptorPool{};
        };

        struct TextureBindingInfo {
            int hasAlbedo{true};
            int hasNormal{false};
            int hasSpecular{false};
            int hasBump{false};
        };

        Mesh(Device& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        Mesh(Device& device, const Builder& builder);

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;

        [[nodiscard]] uint32_t getVertexCount() const { return m_vertexCount; }
        [[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }

        void bind(VkCommandBuffer commandBuffer) const;
        void draw(VkCommandBuffer commandBuffer) const;

        static std::unique_ptr<Mesh> createModelFromFile(
            Device& device, const std::string& filepath);

        Transform& getTransform() { return m_transform; }
        [[nodiscard]] const AABB& GetBoundingBox() const { return m_boundingBox; }

        [[nodiscard]] VkDescriptorSet getDescriptorSet() const {
            return m_descriptorSet;
        }

    private:
        void createVertexBuffer(const std::vector<Vertex>& vertices);
        void createIndexBuffer(const std::vector<uint32_t>& indices);
        void loadTexture(const Material& textureInfo, DescriptorSetLayout* descriptorSetLayout, DescriptorPool* descriptorPool);

        Device& m_device;
        uint32_t m_vertexCount;
        std::unique_ptr<Buffer> m_vertexBuffer;

        bool m_usingIndexBuffer{false};
        uint32_t m_indexCount;
        std::unique_ptr<Buffer> m_indexBuffer;

        Image* m_albedoTexture{};
        Image* m_bumpTexture{};
        Image* m_normalTexture{};
        Image* m_specularTexture{};

        std::unique_ptr<Buffer> m_textureBindingInfoBuffer{};

        Transform m_transform;
        AABB m_boundingBox{}; // Add this member

        VkDescriptorSet m_descriptorSet{VK_NULL_HANDLE};
    };
}


#endif //VMESH_H
