#ifndef VMODEL_H
#define VMODEL_H

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "VDevice.h"

class VModel {
public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    VModel(VDevice& device, const std::vector<Vertex>& vertices);
    ~VModel();

    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer) const;

private:
    void createVertexBuffer(const std::vector<Vertex> &vertices);

    VDevice &m_device;
    VkBuffer m_vertexBuffer;
    VmaAllocation m_vertexAllocation;
    uint32_t m_vertexCount;
};


#endif //VMODEL_H
