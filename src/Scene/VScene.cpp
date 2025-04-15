#include "VScene.h"

VScene::VScene(const std::string& name, std::function<void(VScene*)> loadFunction): m_name(name), m_loadFunction(std::move(loadFunction)) {}

void VScene::addGameObject(std::unique_ptr<VGameObject> gameObject) {
    m_gameObjects.push_back(std::move(gameObject));
}

void VScene::addLineSegment(const LineSegment& lineSegment) {
    m_lineSegments.push_back(lineSegment);
}

void VScene::addBezierCurve(const BezierCurve& curve) {
    m_bezierCurves.push_back(curve);
}

void VScene::clearLineSegments() {
    m_lineSegments.clear();
}

void VScene::clearBezierCurves() {
    m_bezierCurves.clear();
}

void VScene::sceneLoad() {
    if (!m_isLoaded) {
        if (m_loadFunction) {
            m_loadFunction(this);
        }
        m_isLoaded = true;
    }
}