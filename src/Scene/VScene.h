#ifndef VSCENE_H
#define VSCENE_H

#include <functional>
#include <memory>
#include <vector>
#include "Rendering/RenderSystems/LineRenderSystem.h"
#include "Scene/VGameObject.h"

class VScene {
public:
    explicit VScene(std::string  name, std::function<void(VScene*)> loadFunction = nullptr);
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

    void setSceneLoadFunction(std::function<void(VScene*)> loadFunction) {
        m_loadFunction = std::move(loadFunction);
    }

    //Function to be called when the scene is switched to, basically lazy loading
    void sceneLoad();

    [[nodiscard]] bool isLoaded() const { return m_isLoaded; }

private:
    std::string m_name;

    std::vector<std::unique_ptr<VGameObject>> m_gameObjects;
    std::vector<LineSegment> m_lineSegments;
    std::vector<BezierCurve> m_bezierCurves;

    std::function<void(VScene*)> m_loadFunction;

    bool m_isLoaded = false;
};

#endif // VSCENE_H