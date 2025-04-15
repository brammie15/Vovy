#ifndef VSCENE_H
#define VSCENE_H

#include <memory>
#include <vector>
#include "VGameObject.h"
#include "Rendering/RenderSystems/LineRenderSystem.h"

class VScene {
public:
    explicit VScene(const std::string& name);
    ~VScene() = default;

    void addGameObject(std::unique_ptr<VGameObject> gameObject);
    void addLineSegment(const LineSegment& lineSegment);
    void addBezierCurve(const BezierCurve& curve);
    
    std::vector<std::unique_ptr<VGameObject>>& getGameObjects() { return m_gameObjects; }
    std::vector<LineSegment>& getLineSegments() { return m_lineSegments; }
    std::vector<BezierCurve>& getBezierCurves() { return m_bezierCurves; }

    [[nodiscard]] std::string getName() const { return m_name; }

    void clearLineSegments();
    void clearBezierCurves();

private:
    std::string m_name;

    std::vector<std::unique_ptr<VGameObject>> m_gameObjects;
    std::vector<LineSegment> m_lineSegments;
    std::vector<BezierCurve> m_bezierCurves;
};

#endif // VSCENE_H