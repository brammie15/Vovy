#ifndef VMODEL_H
#define VMODEL_H

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <memory>
#include <glm/glm.hpp>

#include "VDevice.h"
#include "VBuffer.h"

class VModel {
public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec2 texCoord{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        void loadModel(const std::string& path);
    };

    VModel(VDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    VModel(VDevice& device, const Builder& builder);
    ~VModel();

    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer) const;

    static std::unique_ptr<VModel> createModelFromFile(
          VDevice &device, const std::string &filepath);
private:
    void createVertexBuffer(const std::vector<Vertex> &vertices);
    void createIndexBuffer(const std::vector<uint32_t> &indices);

    VDevice &m_device;
    uint32_t m_vertexCount;
    std::unique_ptr<VBuffer> m_vertexBuffer;

    bool m_usingIndexBuffer{ false };
    uint32_t m_indexCount;
    std::unique_ptr<VBuffer> m_indexBuffer;
};


#endif //VMODEL_H
