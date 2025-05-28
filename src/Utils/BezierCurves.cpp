#include "BezierCurves.h"

#include <filesystem>

void BezierSerializer::writeBezierCurves(const std::string& filename, const std::vector<BezierCurve>& curves) {
    const std::filesystem::path filePath(filename);
    const std::string extension = filePath.extension().string();
    if (extension != ".bram") {
        throw std::runtime_error("Invalid file extension. Expected .bram");
    }

    std::ofstream out(filename, std::ios::binary);
    if (!out) return;

    const FileHeader header;
    out.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

    const int curveCount = static_cast<int>(curves.size());
    out.write(reinterpret_cast<const char*>(&curveCount), sizeof(int));

    for (const auto& curve : curves) {
        int nodeCount = static_cast<int>(curve.nodes.size());
        out.write(reinterpret_cast<const char*>(&nodeCount), sizeof(int));

        for (const auto& node : curve.nodes) {
            out.write(reinterpret_cast<const char*>(&node.position), sizeof(glm::vec3));
        }

        out.write(reinterpret_cast<const char*>(&curve.color), sizeof(glm::vec3));
        out.write(reinterpret_cast<const char*>(&curve.resolution), sizeof(int));
    }

    out.close();
}

std::vector<BezierCurve> BezierSerializer::readBezierCurves(const std::string& filename) {
    std::filesystem::path filePath(filename);
    std::string extension = filePath.extension().string();
    if (extension != ".bram") {
        throw std::runtime_error("Invalid file extension. Expected .bram");
    }

    std::vector<BezierCurve> curves;

    std::ifstream in(filename, std::ios::binary);
    if (!in) return curves;

    FileHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

    if (!header.isValid()) {
        throw std::runtime_error("Invalid file header signature");
    }

    if (header.version != 1) {
        throw std::runtime_error("Unsupported file version");
    }

    int curveCount;
    in.read(reinterpret_cast<char*>(&curveCount), sizeof(int));

    for (int i = 0; i < curveCount; ++i) {
        BezierCurve curve;

        int nodeCount;
        in.read(reinterpret_cast<char*>(&nodeCount), sizeof(int));

        curve.nodes.resize(nodeCount);
        for (int j = 0; j < nodeCount; ++j) {
            glm::vec3 pos;
            in.read(reinterpret_cast<char*>(&pos), sizeof(glm::vec3));
            curve.nodes[j] = BezierNode(pos);
        }

        in.read(reinterpret_cast<char*>(&curve.color), sizeof(glm::vec3));
        in.read(reinterpret_cast<char*>(&curve.resolution), sizeof(int));

        curves.push_back(std::move(curve));
    }

    in.close();
    return curves;
}
