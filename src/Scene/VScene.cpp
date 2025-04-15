#include "VScene.h"

VScene::VScene(const std::string& name): m_name{name} {}

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