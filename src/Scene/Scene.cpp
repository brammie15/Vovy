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

    void Scene::addPointLight(std::unique_ptr<PointLight> pointLight) {
        m_pointLights.push_back(std::move(pointLight));
    }

    AABB Scene::CalculateSceneAABB() const {
        AABB sceneAABB{};
        bool first = true;

        // Process all game objects
        for (const auto& gameObject : m_gameObjects) {
            // If the game object has a model
            if (gameObject->model) {
                AABB objectAABB = gameObject->model->GetBoundingBox();
                if (first) {
                    sceneAABB = objectAABB;
                    first = false;
                } else {
                    sceneAABB.min = glm::min(sceneAABB.min, objectAABB.min);
                    sceneAABB.max = glm::max(sceneAABB.max, objectAABB.max);
                }
            }
        }

        // If no objects were found, return an empty AABB
        if (first) {
            sceneAABB.min = glm::vec3(0.0f);
            sceneAABB.max = glm::vec3(0.0f);
        }

        return sceneAABB;
    }

    void Scene::clearLineSegments() {
        m_lineSegments.clear();
    }

    void Scene::clearBezierCurves() {
        m_bezierCurves.clear();
    }

    void Scene::SceneLoad() {
        if (!m_isLoaded) {
            if (m_loadFunction) {
                m_loadFunction(this);
            }
            m_isLoaded = true;
        }
    }

    void Scene::SceneUnLoad() {
        m_gameObjects.clear();
        m_lineSegments.clear();
        m_bezierCurves.clear();
        m_isLoaded = false;
    }
}
