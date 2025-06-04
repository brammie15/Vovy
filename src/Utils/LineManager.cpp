#include "LineManager.h"

#include "AABB.h"
#include "AABB.h"
#include "AABB.h"
#include "AABB.h"

namespace vov {

    std::vector<VkVertexInputBindingDescription> vov::LineManager::Vertex::GetBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> vov::LineManager::Vertex::GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});

        return attributeDescriptions;
    }

    void LineManager::AddLine(const glm::vec3& start, const glm::vec3& end) {
        const auto startVert = Vertex(start, glm::vec3(1.0f, 0.0f, 0.0f));
        const auto endVert = Vertex{end, glm::vec3(1.0f, 0.0f, 0.0f)};
        lines.emplace_back(Line{startVert, endVert});
    }

    void LineManager::AddLine(const Line& line) {
        lines.emplace_back(line);
    }

    const std::vector<LineManager::Line>& LineManager::GetLines() const {
        return lines;
    }

    void LineManager::clear() {
        lines.clear();
    }
}
