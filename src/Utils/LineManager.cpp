#include "LineManager.h"

#include "AABB.h"


#include <glm/glm.hpp>

#include "glm/gtc/constants.hpp"
#include "glm/gtx/transform.hpp"

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

    void LineManager::DrawWireSphere(glm::vec3 center, float radius, int segments) {
        for (int i = 0; i < segments; ++i) {
        float theta1 = (i * glm::two_pi<float>()) / float(segments);
        float theta2 = ((i + 1) * glm::two_pi<float>()) / float(segments);

        for (int j = 0; j <= segments; ++j) {
            // phi from –π/2 to +π/2
            float phi = (j * glm::pi<float>()) / float(segments) - glm::half_pi<float>();

            // on a unit sphere (before scaling by radius):
            //   x = cos(phi) * cos(theta)
            //   y = sin(phi)
            //   z = cos(phi) * sin(theta)
            glm::vec3 p1 = {
                radius * cosf(phi) * cosf(theta1),
                radius * sinf(phi),
                radius * cosf(phi) * sinf(theta1)
            };
            glm::vec3 p2 = {
                radius * cosf(phi) * cosf(theta2),
                radius * sinf(phi),
                radius * cosf(phi) * sinf(theta2)
            };

            AddLine(center + p1, center + p2);
        }
    }


    // —— 2) Vertical loops around X‐axis ——
    //
    // Now we draw a second family of great circles, each of which lies in a plane
    // containing the X‐axis.  If you “look along +X,” these are vertical loops in the Y–Z plane.
    // We reuse exactly the same idea as above, but swap roles of 'x' and 'y'.
    for (int i = 0; i < segments; ++i) {
        float theta1 = (i * glm::two_pi<float>()) / float(segments);
        float theta2 = ((i + 1) * glm::two_pi<float>()) / float(segments);

        for (int j = 0; j <= segments; ++j) {
            // phi runs from –π/2 (–90°) to +π/2 (+90°) around the loop:
            float phi = (j * glm::pi<float>()) / float(segments) - glm::half_pi<float>();

            // For a circle in the Y–Z plane (fixed X = 0), the unit‐sphere parameterization is:
            //   x = sin(phi)         // “latitude” tilt becomes the X‐coordinate
            //   y = cos(phi) * cos(theta)
            //   z = cos(phi) * sin(theta)
            //
            // Geometrically:
            //   • When phi = 0, (x,y,z) = (0, cos(0)*cos(theta), cos(0)*sin(theta)) = (0, cos(theta), sin(theta)) → a unit circle in the Y–Z plane.
            //   • As phi → ±π/2, x → ±1 and y,z→0, which is the “top”/“bottom” of that vertical loop.
            glm::vec3 p1 = {
                radius * sinf(phi),
                radius * cosf(phi) * cosf(theta1),
                radius * cosf(phi) * sinf(theta1)
            };
            glm::vec3 p2 = {
                radius * sinf(phi),
                radius * cosf(phi) * cosf(theta2),
                radius * cosf(phi) * sinf(theta2)
            };

            AddLine(center + p1, center + p2);
        }
    }
    }
}
