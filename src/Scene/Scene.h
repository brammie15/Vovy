#ifndef SCENE_H
#define SCENE_H

#include <functional>
#include <memory>
#include <vector>
#include "DirectionalLight.h"
#include "Rendering/RenderSystems/LineRenderSystem.h"
#include "Scene/GameObject.h"

namespace vov {
    class Scene {
    public:
        explicit Scene(std::string name, std::function<void(Scene*)> loadFunction = nullptr);
        ~Scene() = default;

        void addGameObject(std::unique_ptr<GameObject> gameObject);
        void addLineSegment(const LineSegment& lineSegment);
        void addBezierCurve(const BezierCurve& curve);

        std::vector<std::unique_ptr<GameObject>>& getGameObjects() { return m_gameObjects; }
        std::vector<LineSegment>& getLineSegments() { return m_lineSegments; }
        std::vector<BezierCurve>& getBezierCurves() { return m_bezierCurves; }

        [[nodiscard]] std::string getName() const { return m_name; }

        void clearLineSegments();
        void clearBezierCurves();

        void setSceneLoadFunction(std::function<void(Scene*)> loadFunction) {
            m_loadFunction = std::move(loadFunction);
        }

        //Function to be called when the scene is switched to, basically lazy loading
        void sceneLoad();

        [[nodiscard]] bool isLoaded() const { return m_isLoaded; }

        DirectionalLight& getDirectionalLight() {
            return m_directionalLight;
        }

    private:
        std::string m_name;

        std::vector<std::unique_ptr<GameObject>> m_gameObjects;
        std::vector<LineSegment> m_lineSegments;
        std::vector<BezierCurve> m_bezierCurves;

        DirectionalLight m_directionalLight{};

        std::function<void(Scene*)> m_loadFunction;

        bool m_isLoaded = false;
    };
}

#endif // SCENE_H
