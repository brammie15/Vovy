#include "Scene/Lights/PointLight.h"
#include "imgui.h"

vov::PointLight::PointLight(const glm::vec3& position): m_transform(position),
                                                        m_color(1.0f, 1.0f, 1.0f),
                                                        m_intensity(1.0f),
                                                        m_range(10.0f) {
    m_transform.SetWorldPosition(position);
}

void vov::PointLight::RenderImGui() {
    ImGui::ColorEdit3("Color", &m_color.x);
    ImGui::DragFloat("Intensity", &m_intensity, 0.1f);
    ImGui::DragFloat("Range", &m_range, 0.1f);
}
