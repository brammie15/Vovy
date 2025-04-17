#include "Scene.h"

#include <utility>
namespace vov {
    Scene::Scene(std::string name, std::function<void(Scene*)> loadFunction): m_name(std::move(name)), m_loadFunction(std::move(loadFunction)) {}

    void Scene::addGameObject(std::unique_ptr<GameObject> gameObject) {
        m_gameObjects.push_back(std::move(gameObject));
    }

    void Scene::addLineSegment(const LineSegment& lineSegment) {
        m_lineSegments.push_back(lineSegment);
    }

    void Scene::addBezierCurve(const BezierCurve& curve) {
        m_bezierCurves.push_back(curve);
    }

    void Scene::clearLineSegments() {
        m_lineSegments.clear();
    }

    void Scene::clearBezierCurves() {
        m_bezierCurves.clear();
    }

    void Scene::sceneLoad() {
        if (!m_isLoaded) {
            if (m_loadFunction) {
                m_loadFunction(this);
            }
            m_isLoaded = true;
        }
    }
}