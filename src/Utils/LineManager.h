#ifndef LINEMANAGER_H
#define LINEMANAGER_H
#include "Singleton.h"

#include <vector>

#include "Rendering/Passes/LinePass.h"

namespace vov {
    class LineManager final: public Singleton<LineManager> {
    public:
        friend class Singleton<LineManager>;
        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};

            static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
            Vertex() = default;
            Vertex(const glm::vec3& start, const glm::vec3 color)
                : position(start), color(color) {}
        };

        struct Line {
            Vertex start;
            Vertex end;
        };


        void AddLine(const glm::vec3& start, const glm::vec3& end);
        void AddLine(const Line& line);

        const std::vector<Line>& GetLines() const;

        void clear();

        void DrawWireSphere(glm::vec3 center, float radius, int segments = 16);
        void DrawDeathStar(glm::vec3 center, float radius, int segments = 16);

    private:
        LineManager() = default;

        std::vector<Line> lines;
    };
}

#endif // LINEMANAGER_H
