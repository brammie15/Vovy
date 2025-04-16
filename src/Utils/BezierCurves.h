#ifndef BEZIERCURVES_H
#define BEZIERCURVES_H
#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include <glm/vec3.hpp>

struct BezierNode {
    glm::vec3 position;
};

struct BezierCurve {
    std::vector<BezierNode> nodes;
    glm::vec3 color;
    int resolution;
};

struct FileHeader {
    char signature[13]{};     // "Bram Was Here\0"
    uint32_t version = 1;   // 1

    FileHeader() {
        std::memcpy(signature, "Bram Was Here", sizeof(signature));
    }

    [[nodiscard]] bool isValid() const {
        return std::strncmp(signature, "Bram Was Here", sizeof(signature)) == 0;
    }
};

class BezierSerializer {
public:
    static void writeBezierCurves(const std::string& filename, const std::vector<BezierCurve>& curves);
    static std::vector<BezierCurve> readBezierCurves(const std::string& filename);
};



#endif //BEZIERCURVES_H
